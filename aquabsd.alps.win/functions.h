#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FATAL_ERROR(...) fprintf(stderr, "[aquabsd.alps.win] FATAL ERROR "__VA_ARGS__); delete(win); return NULL;
#define WARNING(...) fprintf(stderr, "[aquabsd.alps.win] WARNING "__VA_ARGS__);

// helper functions (for XCB)

static inline xcb_atom_t get_intern_atom(win_t* win, const char* name) {
	// TODO obviously, this function isn't super ideal for leveraging the benefits XCB provides over Xlib
	//      at some point, refactor this so that... well all this work converting from Xlib to XCB isn't for nothing

	xcb_intern_atom_cookie_t atom_cookie = xcb_intern_atom(win->connection, 0, strlen(name), name);
	return xcb_intern_atom_reply(win->connection, atom_cookie, NULL)->atom;
}

static inline char* atom_to_str(win_t* win, xcb_atom_t atom) {
	if (!atom) {
		return NULL;
	}

	xcb_get_property_cookie_t cookie = xcb_get_property(win->connection, 0, win->win, atom, XCB_ATOM_STRING, 0, ~0);

	xcb_generic_error_t* error;
	xcb_get_property_reply_t* reply = xcb_get_property_reply(win->connection, cookie, &error);

	if (error) {
		return NULL;
	}

	unsigned len = xcb_get_property_value_length(reply);
	char* str = calloc(1, len + 1); // +1 because no guarantee this value is null-terminated
	memcpy(str, xcb_get_property_value(reply), len);

	return str;
}

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

static int kbd_update_callback(aquabsd_alps_kbd_t* kbd, void* _win) {
	win_t* win = _win;

	memcpy(kbd->buttons, win->kbd_buttons, sizeof kbd->buttons);

	kbd->buf_len = win->kbd_buf_len;
	kbd->buf = win->kbd_buf;

	// we're passing this on to the keyboard device, so reset

	win->kbd_buf_len = 0;
	win->kbd_buf = NULL;

	return 0;
}

static int x11_kbd_map(xcb_keycode_t key) {
	switch (key) {
		case 9: return AQUABSD_ALPS_KBD_BUTTON_ESC;

		case 111: return AQUABSD_ALPS_KBD_BUTTON_UP;
		case 116: return AQUABSD_ALPS_KBD_BUTTON_DOWN;
		case 113: return AQUABSD_ALPS_KBD_BUTTON_LEFT;
		case 114: return AQUABSD_ALPS_KBD_BUTTON_RIGHT;
	}

	return -1;
}

static win_t* __create_setup(void) {
	win_t* win = calloc(1, sizeof *win);

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

	// get screen

	xcb_screen_iterator_t it = xcb_setup_roots_iterator(xcb_get_setup(win->connection));
	for (int i = win->default_screen; it.rem && i > 0; i--, xcb_screen_next(&it));

	win->screen = it.data;

	// get information about the window manager (i.e. the root window)

	xcb_window_t root = win->screen->root;

	xcb_get_geometry_cookie_t root_geom_cookie = xcb_get_geometry(win->connection, root);
	xcb_get_geometry_reply_t* root_geom = xcb_get_geometry_reply(win->connection, root_geom_cookie, NULL);

	win->wm_x_res = root_geom->width;
	win->wm_y_res = root_geom->height;

	// register a new mouse

	if (aquabsd_alps_mouse_register_mouse) {
		aquabsd_alps_mouse_register_mouse("aquabsd.alps.win mouse", mouse_update_callback, win, 1);
	}

	// register a new keyboard

	if (aquabsd_alps_kbd_register_kbd) {
		aquabsd_alps_kbd_register_kbd("aquabsd.alps.win keyboard", kbd_update_callback, win, 1);
	}

	return win;
}

static void _get_ewmh_atoms(win_t* win) {
	win->_net_wm_visible_name_atom = get_intern_atom(win, "_NET_WM_VISIBLE_NAME");
	win->_net_wm_name_atom = get_intern_atom(win, "_NET_WM_NAME"); // EWMH spec says it's better to use this than just 'WM_NAME'
}

dynamic win_t* create(unsigned x_res, unsigned y_res) {
	win_t* win = __create_setup();

	if (!win) {
		return NULL;
	}

	// create window

	win->x_res = x_res;
	win->y_res = y_res;

	const uint32_t win_attribs[] = {
		XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE,
	};

	win->win = xcb_generate_id(win->connection);

	xcb_create_window(
		win->connection, XCB_COPY_FROM_PARENT, win->win, win->screen->root,
		0, 0, win->x_res, win->y_res, 0, // window geometry
		XCB_WINDOW_CLASS_INPUT_OUTPUT, win->screen->root_visual,
		XCB_CW_EVENT_MASK, win_attribs);

	// get the atoms we'll probably need

	win->wm_protocols_atom = get_intern_atom(win, "WM_PROTOCOLS");
	win->wm_delete_win_atom = get_intern_atom(win, "WM_DELETE_WINDOW");

	_get_ewmh_atoms(win);

	// setup 'WM_DELETE_WINDOW' protocol (yes this is dumb, thank you XCB & X11)
	// also show we support all those other atoms

	xcb_icccm_set_wm_protocols(win->connection, win->win, win->wm_protocols_atom, 1, &win->wm_delete_win_atom);

	// set sensible minimum and maximum sizes for the window

	xcb_size_hints_t hints = { 0 };

	xcb_icccm_size_hints_set_min_size(&hints, 320, 200);
	// no maximum size

	xcb_icccm_set_wm_size_hints(win->connection, win->win, XCB_ATOM_WM_NORMAL_HINTS, &hints);

	// finally (at least for the windowing part), map the window (show it basically) and flush

	xcb_map_window(win->connection, win->win);
	xcb_flush(win->connection);

	win->visible = 1;

	return win;
}

dynamic int set_caption(win_t* win, const char* caption) {
	xcb_change_property(win->connection, XCB_PROP_MODE_REPLACE, win->win, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(caption) /* don't need to include null */, caption);

	xcb_change_property(win->connection, XCB_PROP_MODE_REPLACE, win->win, win->_net_wm_name_atom, XCB_ATOM_STRING, 8, strlen(caption) /* don't need to include null */, caption);
	xcb_change_property(win->connection, XCB_PROP_MODE_REPLACE, win->win, win->_net_wm_visible_name_atom, XCB_ATOM_STRING, 8, strlen(caption) /* don't need to include null */, caption);

	xcb_flush(win->connection);

	return 0;
}

dynamic char* get_caption(win_t* win) {
	char* caption = NULL;

	// first try '_NET_WM_VISIBLE_NAME'

	if ((caption = atom_to_str(win, win->_net_wm_visible_name_atom))) {
		return caption;
	}

	// then, '_NET_WM_NAME'

	if ((caption = atom_to_str(win, win->_net_wm_name_atom))) {
		return caption;
	}

	// if all else fails, try 'WM_NAME'

	if ((caption = atom_to_str(win, XCB_ATOM_WM_NAME))) {
		return caption;
	}

	return NULL;
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

// process a specific event

static int __process_event(win_t* win, xcb_generic_event_t* event, int type) {
	if (type == XCB_EXPOSE) {
		call_cb(win, CB_DRAW);
		invalidate(win);
	}

	else if (type == XCB_CLIENT_MESSAGE) {
		xcb_client_message_event_t* specific = (void*) event;

		if (specific->data.data32[0] == win->wm_delete_win_atom) {
			return -1;
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

		// cf. process_events

		if (win->wm_event_cb) {
			win->wm_prev_button = button;
		}
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

	// keyboard button press/release events

	else if (type == XCB_KEY_PRESS) {
		xcb_key_press_event_t* detail = (void*) event;
		xcb_keycode_t key = detail->detail;

		int index = x11_kbd_map(key);

		if (index >= 0) {
			win->kbd_buttons[index] = 1;
		}

		// get unicode character and append it to the buffer
		// XCB annoyingly doesn't have a function to do this, so we'll need to use Xlib to help us
		// I'll stop my comment right here before I start ranting about XCB

		XKeyEvent xlib_event;

		xlib_event.display = win->display;
		xlib_event.keycode = detail->detail;
		xlib_event.state = detail->state;

		int len = XLookupString(&xlib_event, NULL, 0, NULL, NULL);

		if (len <= 0) {
			return 0;
		}

		unsigned prev_buf_len = win->kbd_buf_len;
		win->kbd_buf_len += len;

		win->kbd_buf = realloc(win->kbd_buf, win->kbd_buf_len);
		XLookupString(&xlib_event, win->kbd_buf + prev_buf_len, len, NULL, NULL);
	}

	else if (type == XCB_KEY_RELEASE) {
		xcb_key_release_event_t* detail = (void*) event;
		xcb_keycode_t key = detail->detail;

		int index = x11_kbd_map(key);

		if (index >= 0) {
			win->kbd_buttons[index] = 0;
		}
	}

	// if we've got 'wm_event_cb', this means a window manager is attached to us
	// pass on all the other events we receive to it, it probably knows better what to do with them than us

	if (win->wm_event_cb) {
		return win->wm_event_cb(win->wm_object, type, event);
	}

	return 0;
}

// poll for events until there are none left (fancy wrapper around __process_event basically)

static int process_events(win_t* win) {
	for (xcb_generic_event_t* event; (event = xcb_poll_for_event(win->connection)); free(event)) {
		int type = event->response_type & XCB_EVENT_RESPONSE_TYPE_MASK;

		if (type == XCB_EXPOSE) {
			// if we've got 'wm_event_cb', this means a window manager is attached to us
			// XXX because of obscure reasons I'm not too sure of (read: not at all sure of), XCB window managers won't catch release events, *sometimes*
			//     works fine with Xlib (cf. x-compositing-wm), but idk XCB just doesn't
			//     and because XCB and X11 in general is a steaming pile of shit documentation-wise and in terms of debugging, this is the solution I've came up with for the meantime 💩
			// TODO try putting this back before the for loop because it seems it only does *not* want to break when running as a WM... something I'm missing perhaps?
			
			if (win->wm_event_cb) {
				xcb_query_pointer_cookie_t cookie = xcb_query_pointer(win->connection, win->win);
				
				xcb_generic_error_t* error;
				xcb_query_pointer_reply_t* reply = xcb_query_pointer_reply(win->connection, cookie, &error);

				if (!error) { // simulate XCB_MOTION_NOTIFY
					xcb_motion_notify_event_t event = {
						.event_x = reply->root_x,
						.event_y = reply->root_y,
					};

					__process_event(win, (xcb_generic_event_t*) &event, XCB_MOTION_NOTIFY);
				}

				if (!error && reply->mask == 0 && win->wm_prev_button) { // simulate XCB_BUTTON_RELEASE
					xcb_button_release_event_t event = {
						.detail = win->wm_prev_button,
					};

					win->wm_prev_button = 0;
					__process_event(win, (xcb_generic_event_t*) &event, XCB_BUTTON_RELEASE);
				}
			}
		}

		if (__process_event(win, event, type) < 0) {
			return 0;
		}
	}

	return 1;
}

dynamic int loop(win_t* win) {
	while (process_events(win));
	return 0; // no more events to process
}

dynamic int grab_focus(win_t* win) {
	// this could be helpful in the future:
	// https://github.com/Cloudef/monsterwm-xcb/blob/master/monsterwm.c

	xcb_set_input_focus(win->connection, XCB_INPUT_FOCUS_PARENT, win->win, XCB_CURRENT_TIME);
	xcb_configure_window(win->connection, win->win, XCB_CONFIG_WINDOW_STACK_MODE, (unsigned[]) { XCB_STACK_MODE_ABOVE });

	return 0;
}

dynamic int move(win_t* win, float x, float y) {
	const uint32_t transformed[] = {
		x * win->wm_x_res,
		(1 - y) * win->wm_y_res - win->y_res,
	};

	xcb_configure_window(win->connection, win->win, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, transformed);
	return 0;
}

dynamic int resize(win_t* win, float x, float y) {
	const uint32_t transformed[] = {
		x * win->wm_x_res,
		y * win->wm_y_res,
	};

	xcb_configure_window(win->connection, win->win, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, transformed);
	return 0;
}

dynamic int get_x_pos(win_t* win) {
	return win->x_pos;
}

dynamic int get_y_pos(win_t* win) {
	return win->y_pos;
}

dynamic int get_x_res(win_t* win) {
	return win->x_res;
}

dynamic int get_y_res(win_t* win) {
	return win->y_res;
}

dynamic int get_wm_x_res(win_t* win) {
	return win->wm_x_res;
}

dynamic int get_wm_y_res(win_t* win) {
	return win->wm_y_res;
}

// functions exposed exclusively to devices

dynamic win_t* create_setup(void) {
	return __create_setup();
}

dynamic void get_ewmh_atoms(win_t* win) {
	_get_ewmh_atoms(win);
}

dynamic int register_dev_cb(win_t* win, cb_t type, int (*cb) (win_t* win, void* param, uint64_t cb, uint64_t cb_param), void* param) {
	if (type >= CB_LEN) {
		fprintf(stderr, "[aquabsd.alps.win] Callback type %d doesn't exist\n", type);
		return -1;
	}

	win->dev_cbs[type] = cb;
	win->dev_cb_params[type] = param;

	return 0;
}

dynamic xcb_window_t get_draw_win(win_t* win) {
	if (win->auxiliary) {
		return win->auxiliary;
	}

	return win->win;
}