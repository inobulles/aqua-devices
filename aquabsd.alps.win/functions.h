#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FATAL_ERROR(...) fprintf(stderr, "[aquabsd.alps.win] FATAL ERROR "__VA_ARGS__); delete(win); return NULL;
#define WARNING(...) fprintf(stderr, "[aquabsd.alps.win] WARNING "__VA_ARGS__);

// functions exposed to devices & apps

dynamic int delete(win_t* win) {
	if (win->display) XCloseDisplay(win->display);

	free(win);

	return 0;
}

static int mouse_update_callback(aquabsd_alps_mouse_t* mouse, void* _win) {
	win_t* win = _win;

	win->mouse_axes[AQUABSD_ALPS_MOUSE_AXIS_X] =       (float) win->mouse_x / win->x_res;
	win->mouse_axes[AQUABSD_ALPS_MOUSE_AXIS_Y] = 1.0 - (float) win->mouse_y / win->y_res;

	memcpy(mouse->buttons, win->mouse_buttons, sizeof mouse->buttons);
	memcpy(mouse->axes, win->mouse_axes, sizeof mouse->axes);

	memset(win->mouse_axes, 0, sizeof win->mouse_axes);

	return 0;
}

dynamic win_t* create(unsigned x_res, unsigned y_res) {
	win_t* win = calloc(1, sizeof *win);

	win->x_res = x_res;
	win->y_res = y_res;

	// get connection to X server

	win->display = XOpenDisplay(NULL /* default to 'DISPLAY' environment variable */);

	if (!win->display) {
		FATAL_ERROR("Failed to open X display\n")
	}

	win->default_screen = DefaultScreen(win->display);
	win->connection = XGetXCBConnection(win->display);

	if (!win->connection) {
		FATAL_ERROR("Failed to get XCB connection from X display\n")
	}

	XSetEventQueueOwner(win->display, XCBOwnsEventQueue);
	
	xcb_screen_iterator_t it = xcb_setup_roots_iterator(xcb_get_setup(win->connection));
	for (int i = win->default_screen; it.rem && i > 0; i--, xcb_screen_next(&it));
	
	win->screen = it.data;

	// create window
	
	const uint32_t win_attribs[] = {
		XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_POINTER_MOTION,
	};

	win->win = xcb_generate_id(win->connection);

	xcb_create_window(
		win->connection, XCB_COPY_FROM_PARENT, win->win, win->screen->root,
		0, 0, win->x_res, win->y_res, 0, // window geometry
		XCB_WINDOW_CLASS_INPUT_OUTPUT, win->screen->root_visual,
		XCB_CW_EVENT_MASK, win_attribs);
	
	// setup 'WM_DELETE_WINDOW' protocol (yes this is dumb, thank you XCB & X11)

	xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom(win->connection, 1, 12 /* strlen("WM_PROTOCOLS") */, "WM_PROTOCOLS");
	xcb_atom_t wm_protocols_atom = xcb_intern_atom_reply(win->connection, wm_protocols_cookie, 0)->atom;

	xcb_intern_atom_cookie_t wm_delete_win_cookie = xcb_intern_atom(win->connection, 0, 16 /* strlen("WM_DELETE_WINDOW") */, "WM_DELETE_WINDOW");
	win->wm_delete_win_atom = xcb_intern_atom_reply(win->connection, wm_delete_win_cookie, 0)->atom;

	xcb_icccm_set_wm_protocols(win->connection, win->win, wm_protocols_atom, 1, &win->wm_delete_win_atom);

	// set sensible minimum and maximum sizes for the window

	xcb_size_hints_t hints = { 0 };

	xcb_icccm_size_hints_set_min_size(&hints, 320, 200);
	// no maximum size

	xcb_icccm_set_wm_size_hints(win->connection, win->win, XCB_ATOM_WM_NORMAL_HINTS, &hints);

	// finally (at least for the windowing part), map the window

	xcb_map_window(win->connection, win->win);

	// register a new mouse

	if (mouse_device != -1) {
		aquabsd_alps_mouse_register_mouse("aquabsd.alps.win mouse", mouse_update_callback, win, 1);
	}

	return win;
}

dynamic int set_caption(win_t* win, const char* caption) {
	xcb_change_property(win->connection, XCB_PROP_MODE_REPLACE, win->win, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(caption) + 1, caption);
	return 0;
}

dynamic int register_cb(win_t* win, cb_t type, uint64_t cb, uint64_t param) {
	if (type >= CB_LEN) {
		fprintf(stderr, "[aquabsd.alps.win] Callback type %d doesn't exist\n", type);
		return -1;
	}

	win->cbs[type] = cb;
	win->cb_params[type] = param;

	return 0;
}

// basically, what we're tryna do here is pass the callback given to the window for a specific action, and pass it on to the device callback (if there is one)
// this means that the device can choose what to do before & after calling the original callback registered to the window

static inline int call_cb(win_t* win, cb_t type) {
	uint64_t cb = win->cbs[type];
	uint64_t param = win->cb_params[type];

	if (win->dev_cbs[type]) {
		void* dev_param = win->dev_cb_params[type];
		return win->dev_cbs[type](win, dev_param, cb, param);
	}

	if (!cb) {
		return -1;
	}

	return kos_callback(cb, 2, (uint64_t) win, param);
}

static void invalidate(win_t* win) {
	xcb_expose_event_t event;

	event.window = win->win;
	event.response_type = XCB_EXPOSE;

	event.x = 0;
	event.y = 0;

	event.width  = win->x_res;
	event.height = win->y_res;

	xcb_send_event(win->connection, 0, win->win, XCB_EVENT_MASK_EXPOSURE, (const char*) &event);
	xcb_flush(win->connection);
}

static int process_events(win_t* win) {
	for (xcb_generic_event_t* event; (event = xcb_poll_for_event(win->connection)); free(event)) {
		int type = event->response_type & XCB_EVENT_RESPONSE_TYPE_MASK;

		if (type == XCB_EXPOSE) {
			call_cb(win, CB_DRAW);
			invalidate(win);
		}

		else if (type == XCB_CLIENT_MESSAGE) {
			xcb_client_message_event_t* specific = (void*) event;

			if (specific->data.data32[0] == win->wm_delete_win_atom) {
				return 0;
			}
		}

		// mouse button press/release events

		else if (type == XCB_BUTTON_PRESS) {
			xcb_button_press_event_t* detail = (void*) event;
			xcb_button_t button = detail->detail;

			if (button == 1) win->mouse_buttons[AQUABSD_ALPS_MOUSE_BUTTON_LEFT  ] = 1;
			if (button == 3) win->mouse_buttons[AQUABSD_ALPS_MOUSE_BUTTON_RIGHT ] = 1;
			if (button == 2) win->mouse_buttons[AQUABSD_ALPS_MOUSE_BUTTON_MIDDLE] = 1;

			if (button == 5) win->mouse_axes[AQUABSD_ALPS_MOUSE_AXIS_Z] = -1.0;
			if (button == 4) win->mouse_axes[AQUABSD_ALPS_MOUSE_AXIS_Z] =  1.0;
		}

		else if (type == XCB_BUTTON_RELEASE) {
			xcb_button_release_event_t* detail = (void*) event;
			xcb_button_t button = detail->detail;

			if (button == 1) win->mouse_buttons[AQUABSD_ALPS_MOUSE_BUTTON_LEFT  ] = 0;
			if (button == 3) win->mouse_buttons[AQUABSD_ALPS_MOUSE_BUTTON_RIGHT ] = 0;
			if (button == 2) win->mouse_buttons[AQUABSD_ALPS_MOUSE_BUTTON_MIDDLE] = 0;
		}

		// mouse motion events

		else if (type == XCB_ENTER_NOTIFY) {
			xcb_enter_notify_event_t* detail = (void*) event;

			win->mouse_x = detail->event_x;
			win->mouse_y = detail->event_y;
		}

		else if (type == XCB_LEAVE_NOTIFY) {
			xcb_leave_notify_event_t* detail = (void*) event;
			
			win->mouse_x = detail->event_x;
			win->mouse_y = detail->event_y;
		}

		else if (type == XCB_MOTION_NOTIFY) {
			xcb_motion_notify_event_t* detail = (void*) event;
			
			win->mouse_x = detail->event_x;
			win->mouse_y = detail->event_y;
		}
	}

	return 1;
}

dynamic int loop(win_t* win) {
	while (process_events(win));
	return 0; // no more events to process
}

// functions exposed exclusively to devices

dynamic int register_dev_cb(win_t* win, cb_t type, int (*cb) (win_t* win, void* param, uint64_t cb, uint64_t cb_param), void* param) {
	if (type >= CB_LEN) {
		fprintf(stderr, "[aquabsd.alps.win] Callback type %d doesn't exist\n", type);
		return -1;
	}

	win->dev_cbs[type] = cb;
	win->dev_cb_params[type] = param;

	return 0;
}