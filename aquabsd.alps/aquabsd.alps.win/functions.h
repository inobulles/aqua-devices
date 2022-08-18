#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include <umber.h>
#define UMBER_COMPONENT "aquabsd.alps.win"

// #define EVENT_THREADING_ENABLED

#define FATAL_ERROR(...) \
	LOG_FATAL(__VA_ARGS__) \
	delete(win); \
	return NULL;

// once we've received SIGINT, we must exit as soon as possible;
// there's no going back

static unsigned sigint_received = 0;

static void sigint_handler(int sig) {
	if (sig != SIGINT) {
		LOG_WARN("Got unexpected signal %s (should only be %s)", strsignal(sig), strsignal(SIGINT))
	}

	LOG_INFO("Received a SIGINT signal")

	sigint_received = 1;

	// unset the signal handler so that it can (perhaps?) act a bit more forcefully

	struct sigaction sa = {
		.sa_handler = SIG_DFL,
	};

	sigaction(SIGINT, &sa, NULL);
}

// helper functions (for XCB)

static inline xcb_atom_t __get_intern_atom(win_t* win, const char* name) {
	// TODO obviously, this function isn't super ideal for leveraging the benefits XCB provides over Xlib
	//      at some point, refactor this so that... well all this work converting from Xlib to XCB isn't for nothing

	xcb_intern_atom_cookie_t atom_cookie = xcb_intern_atom(win->connection, 0 /* only_if_exists: don't create atom if it doesn't already exist */, strlen(name), name);
	xcb_intern_atom_reply_t* atom_reply = xcb_intern_atom_reply(win->connection, atom_cookie, NULL);

	xcb_atom_t atom = atom_reply->atom;
	free(atom_reply);

	return atom;
}

static char* atom_to_str(win_t* win, xcb_atom_t atom) {
	if (!atom) {
		return NULL;
	}

	xcb_get_property_cookie_t cookie = xcb_get_property(win->connection, 0, win->win, atom, XCB_ATOM_STRING, 0, ~0);

	xcb_generic_error_t* error;
	xcb_get_property_reply_t* reply = xcb_get_property_reply(win->connection, cookie, &error);

	if (error) {
		free(error);
		return NULL;
	}

	// TODO evidently, I'm doing something incorrect here, as I recorded a crash in xcb_get_property_value_length or xcb_get_property_value (could be a thread safety issue? idk)

	unsigned len = xcb_get_property_value_length(reply);
	char* str = calloc(1, len + 1); // +1 because no guarantee this value is null-terminated

	memcpy(str, xcb_get_property_value(reply), len);
	free(reply);

	return str;
}

// functions exposed to devices & apps

dynamic int delete(win_t* win) {
	LOG_VERBOSE("%p: Delete window", win);

	if (win->display) {
		XCloseDisplay(win->display);
	}

	if (win->kbd_keys) {
		free(win->kbd_keys);
	}

	free(win);

	return 0;
}

dynamic int set_caption(win_t* win, const char* caption) {
	// TODO replace with xcb_ewmh_set_wm_name

	LOG_VERBOSE("%p: Set caption to \"%s\"", win, caption)

	xcb_change_property(win->connection, XCB_PROP_MODE_REPLACE, win->win, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(caption) /* don't need to include null */, caption);

	xcb_change_property(win->connection, XCB_PROP_MODE_REPLACE, win->win, win->ewmh._NET_WM_NAME, XCB_ATOM_STRING, 8, strlen(caption) /* don't need to include null */, caption);
	xcb_change_property(win->connection, XCB_PROP_MODE_REPLACE, win->win, win->ewmh._NET_WM_VISIBLE_NAME, XCB_ATOM_STRING, 8, strlen(caption) /* don't need to include null */, caption);

	xcb_flush(win->connection);

	return 0;
}

dynamic char* get_caption(win_t* win) {
	// TODO replace with xcb_ewmh_get_wm_name

	LOG_VERBOSE("%p: Get caption", win)

	char* caption = NULL;

	#define TRY_ATOM(atom) \
		caption = atom_to_str(win, (atom)); \
		\
		if (caption && *caption) { \
			LOG_INFO("%p: Window caption is \"%s\"", win, caption) \
			return caption; \
		}

	// first try '_NET_WM_VISIBLE_NAME'
	// then, '_NET_WM_NAME' (EWMH spec says it's better to use this than just 'WM_NAME')
	// if all else fails, try 'WM_NAME'

	TRY_ATOM(win->ewmh._NET_WM_VISIBLE_NAME)
	TRY_ATOM(win->ewmh._NET_WM_NAME)
	TRY_ATOM(XCB_ATOM_WM_NAME)

	LOG_WARN("%p: Could not find caption (checked _NET_WM_VISIBLE_NAME, _NET_WM_NAME, & WM_NAME atoms)", win)

	return NULL;
}

dynamic state_t get_state(win_t* win) {
	return win->state;
}

dynamic unsigned supports_dwd(win_t* win) {
	xcb_get_property_cookie_t cookie = xcb_get_property(win->connection, 0, win->win, win->dwd_supports_atom, XCB_ATOM_INTEGER, 0, ~0);

	xcb_generic_error_t* error;
	xcb_get_property_reply_t* reply = xcb_get_property_reply(win->connection, cookie, &error);

	if (error) {
		free(error);
		return 0;
	}

	win->supports_dwd = *(uint32_t*) xcb_get_property_value(reply);
	free(reply);

	return win->supports_dwd;
}

static inline void __get_dwd_close_pos(win_t* win) {
	xcb_get_property_cookie_t cookie = xcb_get_property(win->connection, 0, win->win, win->dwd_close_pos_atom, XCB_ATOM_POINT, 0, ~0);

	xcb_generic_error_t* error;
	xcb_get_property_reply_t* reply = xcb_get_property_reply(win->connection, cookie, &error);

	if (error) {
		free(error);
		return;
	}

	memcpy(win->dwd_close_pos, xcb_get_property_value(reply), sizeof win->dwd_close_pos);
	free(reply);
}

dynamic float get_dwd_close_pos_x(win_t* win) {
	__get_dwd_close_pos(win);
	return (float) win->dwd_close_pos[0] / win->x_res;
}

dynamic float get_dwd_close_pos_y(win_t* win) {
	__get_dwd_close_pos(win);
	return 1 - (float) win->dwd_close_pos[1] / win->y_res;
}

static inline void __support_dwd(win_t* win, unsigned flush) {
	// function called for each call to one of the DWD functions
	// by calling them, the client implicitly acknowledged and accepts to partake in the AQUA DWD protocol

	if (win->supports_dwd) {
		return;
	}

	LOG_VERBOSE("%p: Set the _AQUA_DWD_SUPPORTS atom to tell the window manager we support the AQUA DWD protocol", win)

	win->supports_dwd = 1;
	xcb_change_property(win->connection, XCB_PROP_MODE_REPLACE, win->win, win->dwd_supports_atom, XCB_ATOM_INTEGER, 32, 1, &win->supports_dwd);

	if (flush) {
		xcb_flush(win->connection);
	}
}

dynamic int set_dwd_close_pos(win_t* win, float x, float y) {
	__support_dwd(win, 0);

	win->dwd_close_pos[0] =      x  * win->x_res;
	win->dwd_close_pos[1] = (1 - y) * win->y_res;

	LOG_VERBOSE("%p: Set the DWD close button position to %dx%d", win, win->dwd_close_pos[0], win->dwd_close_pos[1])

	xcb_change_property(win->connection, XCB_PROP_MODE_REPLACE, win->win, win->dwd_close_pos_atom, XCB_ATOM_POINT, 16, 2, &win->dwd_close_pos);
	xcb_flush(win->connection);

	return 0;
}

dynamic int register_cb(win_t* win, cb_t type, uint64_t cb, uint64_t param) {
	if (type >= CB_LEN) {
		LOG_WARN("Callback type %d doesn't exist", type)
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

// event stuff

static int mouse_update_callback(aquabsd_alps_mouse_t* mouse, void* _win) {
	win_t* win = _win;

	win->mouse_axes[AQUABSD_ALPS_MOUSE_AXIS_X] =       (float) win->mouse_x / win->x_res;
	win->mouse_axes[AQUABSD_ALPS_MOUSE_AXIS_Y] = 1.0 - (float) win->mouse_y / win->y_res;

	memcpy(mouse->buttons, win->mouse_buttons, sizeof mouse->buttons);
	memcpy(mouse->axes, win->mouse_axes, sizeof mouse->axes);

	if (win->mouse_scroll) {
		mouse->axes[AQUABSD_ALPS_MOUSE_AXIS_Z] = win->mouse_scroll;
	}

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

	// count keys

	kbd->keys_len = 0;

	for (size_t i = 0; i < win->kbd_keys_len; i++) {
		kbd->keys_len += !!win->kbd_keys[i];
	}

	// allocate space for & copy over keys

	kbd->keys = malloc(kbd->keys_len * sizeof *kbd->keys);
	kbd->keys_len = 0;

	for (size_t i = 0; i < win->kbd_keys_len; i++) {
		const char* key = win->kbd_keys[i];

		if (key) {
			kbd->keys[kbd->keys_len++] = key;
		}
	}

	return 0;
}

static int x11_kbd_map(xcb_keycode_t key) {
	// X11, for some reason (which, of course, is not explained anywhere), adds 8 to the standard PC scancode
	// cf. https://unix.stackexchange.com/a/167959

	int i = key - 8;

	if (i < 0 || i > 255) {
		LOG_ERROR("Keycode index (%d) not in [0; 255] range", i)
		return 0;
	}

	return i;
}

static int _close_win(win_t* win) {
	LOG_VERBOSE("%p: Close window (with the WM_DELETE_WINDOW protocol)")

	xcb_client_message_event_t event;

	event.response_type = XCB_CLIENT_MESSAGE;

	event.window = win->win;

	event.format = 32;
	event.sequence = 0;

	event.type = win->wm_protocols_atom;

	event.data.data32[0] = win->wm_delete_win_atom;
	event.data.data32[1] = XCB_CURRENT_TIME;

	xcb_send_event(win->connection, 1 /* propagate, i.e. send this to all children of window */, win->win, XCB_EVENT_MASK_NO_EVENT, (const char*) &event);
	xcb_flush(win->connection);

	// if window manager:
	// yes, this is super dirty and will result in a segfault, but I don't see any other way of doing it
	// window managers completely ignore XCB_CLIENT_MESSAGE events, idk why
	// I've probably spent the last 5 hours working on this problem (as is usually the case with XCB) with no solution, so I give up, not even going to mark this as TODO

	if (win->wm_event_cb && win->event_threading_enabled) {
		win->running = 0;
	}

	return 0;
}

// process a specific event

static int process_event(win_t* win, xcb_generic_event_t* event, int type) {
	// window management events

	if (type == XCB_CLIENT_MESSAGE) {
		xcb_client_message_event_t* detail = (void*) event;

		if (
			detail->data.data32[0] == win->wm_delete_win_atom &&
			detail->window == win->win // make sure it is indeed us someone's trying to kill
		) {
			return -1;
		}
	}

	else if (type == XCB_CONFIGURE_NOTIFY) {
		xcb_configure_notify_event_t* detail = (void*) event;

		bool resize =
			win->x_res != detail->width ||
			win->y_res != detail->height;

		if (detail->window == win->win && resize) {
			win->x_res = detail->width;
			win->y_res = detail->height;

			call_cb(win, CB_RESIZE);
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

	// keyboard button press/release events

	else if (type == XCB_KEY_PRESS) {
		xcb_key_press_event_t* detail = (void*) event;
		xcb_keycode_t key = detail->detail;

		// the keycode is the physical scancode of the button pressed on the keyboard
		// the keysym is the interpretation of the scancode (so depending on keyboard layout and other bindings)

		int index = x11_kbd_map(key);

		if (index >= 0) {
			win->kbd_buttons[index] = 1;
		}

		// get unicode character and append it to the buffer
		// XCB annoyingly doesn't have a function to do this, so we'll need to use Xlib to help us
		// I'll stop my comment right here before I start ranting about XCB (again)

		XKeyEvent xlib_event;

		xlib_event.display = win->display;
		xlib_event.keycode = detail->detail;
		xlib_event.state = detail->state;

		KeySym keysym;
		int len = XLookupString(&xlib_event, NULL, 0, &keysym, NULL);

		// add to keys buffer
		// if the key is already in the keys buffer, we don't need to add it

		const char* aqua_key = aquabsd_alps_kbd_x11_map(keysym);

		for (size_t i = 0; i < win->kbd_keys_len; i++) {
			if (win->kbd_keys[i] == aqua_key) { // we don't need to use 'strcmp'; this is okay 👌
				goto skip;
			}
		}

		// if there's an empty space in the list somewhere, put our key there (instead of reallocating the keys list)

		for (size_t i = 0; i < win->kbd_keys_len; i++) {
			if (win->kbd_keys[i]) {
				continue;
			}

			win->kbd_keys[i] = aqua_key;
			goto skip;
		}

		// if our key is not already in the keys list and there's no empty space, add it to the end of the list

		win->kbd_keys = realloc(win->kbd_keys, ++win->kbd_keys_len * sizeof *win->kbd_keys);
		win->kbd_keys[win->kbd_keys_len - 1] = aqua_key;

	skip:

		// add to string buffer

		if (len <= 0) {
			return 0;
		}

		size_t prev_buf_len = win->kbd_buf_len;
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

		// get X11 keysym

		XKeyEvent xlib_event;

		xlib_event.display = win->display;
		xlib_event.keycode = detail->detail;
		xlib_event.state = detail->state;

		KeySym keysym;
		XLookupString(&xlib_event, NULL, 0, &keysym, NULL);

		// remove from keys buffer

		const char* aqua_key = aquabsd_alps_kbd_x11_map(keysym);

		for (size_t i = 0; i < win->kbd_keys_len; i++) {
			if (win->kbd_keys[i] != aqua_key) {
				continue;
			}

			win->kbd_keys[i] = NULL;
		}

		// if there are any trailing NULL's, reallocate our keys list to make it smaller
		// this is theoretically not super necessary, as the keys list will never be too large

		size_t prev_len = win->kbd_keys_len;

		while (!win->kbd_keys[win->kbd_keys_len - 1] && win->kbd_keys_len --> 1);

		if (!win->kbd_keys_len) {
			free(win->kbd_keys);
			win->kbd_keys = NULL;
		}

		else if (win->kbd_keys_len != prev_len) {
			win->kbd_keys = realloc(win->kbd_keys, win->kbd_keys_len * sizeof *win->kbd_keys);
		}
	}

	// if we've got 'wm_event_cb', this means a window manager is attached to us
	// pass on all the other events we receive to it, it probably knows better what to do with them than us

	if (win->wm_event_cb) {
		return win->wm_event_cb(win->wm_object, type, event);
	}

	return 0;
}

static void process_events(win_t* win, xcb_generic_event_t* (*xcb_event_func) (xcb_connection_t* c)) {
	if (xcb_event_func != xcb_wait_for_event && xcb_event_func != xcb_poll_for_event) {
		LOG_WARN("xcb_event_func passed to %s (%p) must either point to xcb_wait_for_event or xcb_poll_for_event", __func__, xcb_event_func)
	}

	// poll for events until there are none left (fancy wrapper around process_event basically)

	for (xcb_generic_event_t* event; (event = xcb_event_func(win->connection)); free(event)) {
		int type = event->response_type & XCB_EVENT_RESPONSE_TYPE_MASK;

		if (process_event(win, event, type) < 0) {
			win->running = 0;
			break;
		}
	}
}

// event loop

#if !defined(EVENT_THREADING_ENABLED)
	__attribute__((unused))
#endif

static void* event_thread(void* _win) {
	win_t* win = _win;

	LOG_VERBOSE("%p: Start event thread loop", win)

	while (win->running) {
		// we use xcb_wait_for_event here since we're threading;
		// there's no point constantly polling for events because there's nothing else this thread has to do

		process_events(win, xcb_wait_for_event);
	}

	return NULL;
}

// draw loop

dynamic int loop(win_t* win) {
	// don't ever explicitly break out of this loop or return from this function;
	// instead, wait until the event thread has gracefully exitted
	// we call _close_win instead of just setting win->running to wake up the event thread

	LOG_VERBOSE("%p: Start window loop", win)

	while (win->running) {
		// signal events

		if (sigint_received) {
			_close_win(win);
		}

		// actual events (ONLY if no event thread, i.e. threading is disabled)
		// we use xcb_poll_for_event here since we're not threading;
		// if we wait for the next event, there's a chance we'll be blocked here for a while

		if (!win->event_threading_enabled) {
			process_events(win, xcb_poll_for_event);
		}

		// if in a window manager, do all the last minute things it wants to do before letting the client draw

		if (win->wm_predraw_cb) {
			win->wm_predraw_cb(win->wm_object);
		}

		// actually draw

		if (call_cb(win, CB_DRAW) == 1) {
			_close_win(win);
		}

		win->mouse_scroll = win->mouse_axes[AQUABSD_ALPS_MOUSE_AXIS_Z];
		win->mouse_axes[AQUABSD_ALPS_MOUSE_AXIS_Z] = 0;
	}

	return 0;
}

// state modification functions

dynamic int close_win(win_t* win) {
	return _close_win(win);
}

dynamic int grab_focus(win_t* win) {
	// this could be helpful in the future:
	// https://github.com/Cloudef/monsterwm-xcb/blob/master/monsterwm.c

	LOG_VERBOSE("%p: Grab focus", win)

	xcb_set_input_focus(win->connection, XCB_INPUT_FOCUS_PARENT, win->win, XCB_CURRENT_TIME);
	xcb_configure_window(win->connection, win->win, XCB_CONFIG_WINDOW_STACK_MODE, (unsigned[]) { XCB_STACK_MODE_ABOVE });

	xcb_flush(win->connection);

	return 0;
}

dynamic int modify(win_t* win, float x, float y, unsigned x_res, unsigned y_res) {
	int32_t x_px =      x  * win->wm_x_res;
	int32_t y_px = (1 - y) * win->wm_y_res - y_res;

	LOG_VERBOSE("%p: Modify window geometry (%dx%d+%d+%d)", win, x_res, y_res, x_px, y_px)

	const int32_t transformed[] = {
		x_px,  y_px,
		x_res, y_res
	};

	uint32_t config =
		XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
		XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT;

	xcb_configure_window(win->connection, win->win, config, transformed);
	xcb_flush(win->connection);

	return 0;
}

// getter functions

dynamic float get_x_pos(win_t* win) {
	return (float) win->x_pos / win->wm_x_res;
}

dynamic float get_y_pos(win_t* win) {
	int transformed = win->wm_y_res - win->y_pos - win->y_res;
	return (float) transformed / win->wm_y_res;
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

// creation & setup functions

static win_t* _create_setup(void) {
	win_t* win = calloc(1, sizeof *win);

	// get connection to X server

	LOG_VERBOSE("Get connection to X server")

	win->display = XOpenDisplay(NULL /* default to 'DISPLAY' environment variable */);

	if (!win->display) {
		FATAL_ERROR("Failed to open X display")
	}

	win->default_screen = DefaultScreen(win->display);
	win->connection = XGetXCBConnection(win->display);

	if (!win->connection) {
		FATAL_ERROR("Failed to get XCB connection from X display")
	}

	XSetEventQueueOwner(win->display, XCBOwnsEventQueue);

	// get screen

	LOG_VERBOSE("Get screen")

	xcb_screen_iterator_t it = xcb_setup_roots_iterator(xcb_get_setup(win->connection));
	for (int i = win->default_screen; it.rem && i > 0; i--, xcb_screen_next(&it));

	win->screen = it.data;

	// get information about the window manager (i.e. the root window)

	LOG_VERBOSE("Get information about the window manager (i.e. the root window)")

	xcb_window_t root = win->screen->root;

	xcb_get_geometry_cookie_t root_geom_cookie = xcb_get_geometry(win->connection, root);
	xcb_get_geometry_reply_t* root_geom = xcb_get_geometry_reply(win->connection, root_geom_cookie, NULL);

	win->wm_x_res = root_geom->width;
	win->wm_y_res = root_geom->height;

	free(root_geom);

	LOG_INFO("Window manager's total resolution is %dx%d", win->wm_x_res, win->wm_y_res)

	// get the atoms we'll probably need
	// these aren't included in XCB or even the EWMH atoms struct (xcb_ewmh_connection_t) from xcb-ewmh.h for whatever reason

	LOG_VERBOSE("Get atoms")

	win->wm_protocols_atom  = __get_intern_atom(win, "WM_PROTOCOLS");
	win->wm_delete_win_atom = __get_intern_atom(win, "WM_DELETE_WINDOW");

	// setup 'WM_DELETE_WINDOW' protocol (yes, this is dumb, thank you XCB & X11)
	// TODO now that I think of it, is this maybe why window managers don't accept requests to die?

	LOG_VERBOSE("Setup WM_DELETE_WINDOW protocol")

	xcb_icccm_set_wm_protocols(win->connection, win->win, win->wm_protocols_atom, 1, &win->wm_delete_win_atom);

	// get EWMH atoms
	// TODO show we support these atoms? how does this work again?
	// TODO do these atoms need to be freed at some point? somethingsomething_wipe?

	LOG_VERBOSE("Get X11 EWMH atoms")

	xcb_intern_atom_cookie_t* cookies = xcb_ewmh_init_atoms(win->connection, &win->ewmh);

	if (!xcb_ewmh_init_atoms_replies(&win->ewmh, cookies, NULL)) {
		FATAL_ERROR("Failed to get EWMH atoms")
	}

	// get AQUA DWD protocol atoms

	LOG_VERBOSE("Get AQUA DWD protocol atoms")

	win->supports_dwd = 0; // we don't support the DWD protocol by default

	win->dwd_supports_atom  = __get_intern_atom(win, "_AQUA_DWD_SUPPORTS");
	win->dwd_close_pos_atom = __get_intern_atom(win, "_AQUA_DWD_CLOSE_POS");

	// register a new mouse

	LOG_VERBOSE("Register window mouse")

	if (aquabsd_alps_mouse_register_mouse) {
		aquabsd_alps_mouse_register_mouse("aquabsd.alps.win mouse", mouse_update_callback, win, 1);
	}

	// register a new keyboard

	LOG_VERBOSE("Register window keyboard")

	if (aquabsd_alps_kbd_register_kbd) {
		aquabsd_alps_kbd_register_kbd("aquabsd.alps.win keyboard", kbd_update_callback, win, 1);
	}

	// register signal handlers

	LOG_VERBOSE("Register signal handlers")

	struct sigaction sa = {
		.sa_handler = sigint_handler,
	};

	sigaction(SIGINT, &sa, NULL);

	// setup event thread

	LOG_VERBOSE("Setup event thread")

#if defined(EVENT_THREADING_ENABLED)
	win->event_threading_enabled = 1;

	if (pthread_create(&win->event_thread, NULL, event_thread, win) < 0) {
		win->event_threading_enabled = 0;
		LOG_WARN("Failed to create event thread for window (%s) - threading will be disabled", strerror(errno))
	}
#endif

	win->running = 1;

	return win;
}

dynamic win_t* create(unsigned x_res, unsigned y_res) {
	LOG_INFO("Create window (initial resolution of %dx%d)", x_res, y_res)

	win_t* win = _create_setup();

	if (!win) {
		return NULL;
	}

	// create window

	LOG_VERBOSE("Actually create X window")

	win->x_res = x_res;
	win->y_res = y_res;

	const uint32_t win_attribs[] = {
		XCB_EVENT_MASK_EXPOSURE | // probably not all that important anymore
		XCB_EVENT_MASK_STRUCTURE_NOTIFY |
		XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
		XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_POINTER_MOTION |
		XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE,
	};

	win->win = xcb_generate_id(win->connection);

	xcb_create_window(
		win->connection, XCB_COPY_FROM_PARENT, win->win, win->screen->root,
		0, 0, win->x_res, win->y_res, 0, // window geometry
		XCB_WINDOW_CLASS_INPUT_OUTPUT, win->screen->root_visual,
		XCB_CW_EVENT_MASK, win_attribs);

	// set sensible minimum and maximum sizes for the window

	LOG_VERBOSE("Set ICCCM hints (minimum & maximum sizes)")

	xcb_size_hints_t hints = { 0 };

	xcb_icccm_size_hints_set_min_size(&hints, 320, 200);
	// no maximum size

	xcb_icccm_set_wm_size_hints(win->connection, win->win, XCB_ATOM_WM_NORMAL_HINTS, &hints);

	// finally (at least for the windowing part), map the window (show it basically) and flush

	LOG_VERBOSE("Map window and flush everything")

	xcb_map_window(win->connection, win->win);
	xcb_flush(win->connection);

	win->visible = 1;

	LOG_SUCCESS("Window created: %p", win)

	return win;
}

// functions exposed exclusively to devices

dynamic win_t* create_setup(void) {
	return _create_setup();
}

dynamic int register_dev_cb(win_t* win, cb_t type, int (*cb) (win_t* win, void* param, uint64_t cb, uint64_t cb_param), void* param) {
	if (type >= CB_LEN) {
		LOG_WARN("Callback type %d doesn't exist", type)
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
