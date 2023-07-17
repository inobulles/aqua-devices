#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xcb/randr.h>

#include <umber.h>
#define UMBER_COMPONENT "aquabsd.alps.wm"

#define SUPER_MOD XCB_MOD_MASK_4 // looks like this is the super modifier
#define SUPER_KEY 0x85

#define FATAL_ERROR(...) \
	LOG_FATAL(__VA_ARGS__) \
	delete(wm); \
	return NULL;

// helper functions (for XCB)

static inline xcb_window_t create_dumb_win(wm_t* wm) {
	xcb_window_t win = xcb_generate_id(wm->root->connection);
	xcb_create_window(wm->root->connection, XCB_COPY_FROM_PARENT, win, wm->root->win, 0, 0, 1, 1, 0, XCB_WINDOW_CLASS_COPY_FROM_PARENT, 0, 0, NULL);

	return win;
}

// actual functions

dynamic int delete(wm_t* wm) {
	LOG_VERBOSE("%p: Delete window manager", wm)

	if (wm->root) {
		aquabsd_alps_win_delete(wm->root);
	}

	if (wm->providers) {
		free(wm->providers);
	}

	free(wm);

	return 0;
}

static inline int call_cb(wm_t* wm, win_t* win, cb_t type) {
	uint64_t cb = wm->cbs[type];
	uint64_t param = wm->cb_params[type];

	if (!cb) {
		return -1;
	}

	return kos_callback(cb, 3, (uint64_t) wm, (uint64_t) win, param);
}

static void update_client_list(wm_t* wm) {
	LOG_VERBOSE("%p: Update client list", wm)

	xcb_window_t* client_list = calloc(wm->win_count, sizeof *client_list);
	size_t i = 0;

	for (win_t* win = wm->win_head; win; win = win->next) {
		if (i > wm->win_count) {
			LOG_ERROR("%p: Window linked list is larger than wm->win_count (= %d)", wm, wm->win_count)
			break;
		}

		client_list[i++] = win->win;
	}

	xcb_change_property(wm->root->connection, XCB_PROP_MODE_REPLACE, wm->root->win, wm->root->ewmh._NET_CLIENT_LIST, XCB_ATOM_WINDOW, 32, wm->win_count, client_list);
	xcb_flush(wm->root->connection);

	free(client_list);
}

static win_t* add_win(wm_t* wm, xcb_window_t id) {
	win_t* win = calloc(1, sizeof *win);
	LOG_VERBOSE("%p: Add window (ID = 0x%x, shell window is %p) to linked list", wm, id, win)

	// copy over relevant information to the new shell window

	win->connection = wm->root->connection;

	win->root = wm->root->win;
	win->win = id;

	win->wm_protocols_atom  = wm->root->wm_protocols_atom;
	win->wm_delete_win_atom = wm->root->wm_delete_win_atom;

	memcpy(&win->ewmh, &wm->root->ewmh, sizeof win->ewmh);

	// copy over AQUA DWD protocol atoms

	win->dwd_supports_atom  = wm->root->dwd_supports_atom;
	win->dwd_close_pos_atom = wm->root->dwd_close_pos_atom;

	// add window to window linked list

	if (!wm->win_head) {
		wm->win_head = win;
	}

	win->prev = wm->win_tail;

	if (wm->win_tail) {
		wm->win_tail->next = win;
	}

	wm->win_tail = win;

	call_cb(wm, win, CB_CREATE);

	wm->win_count++;
	update_client_list(wm);

	return win;
}

static win_t* search_win(wm_t* wm, xcb_window_t id) {
	// obviously we very rarely care about interacting with the root window

	if (id == wm->root->win) {
		return NULL;
	}

	// in a compositing window manager, this means we're dealing with the overlay
	// prevent that

	if (id == wm->root->auxiliary) {
		LOG_WARN("%p: Window ID (0x%x) is the auxiliary window")
		return NULL;
	}

	for (win_t* win = wm->win_head; win; win = win->next) {
		if (win->win == id) {
			return win;
		}
	}

	LOG_WARN("%p: Window (ID = 0x%x) was not found", wm, id)
	return NULL;
}

static inline void __set_type(wm_t* wm, win_t* win, xcb_atom_t atom) {
	unsigned transient = 1;

	if (
		atom == wm->root->ewmh._NET_WM_WINDOW_TYPE_DESKTOP ||
		atom == wm->root->ewmh._NET_WM_WINDOW_TYPE_NORMAL
	) {
		transient = 0;
	}

	win->state &= ~AQUABSD_ALPS_WIN_STATE_TRANSIENT;
	win->state |=  AQUABSD_ALPS_WIN_STATE_TRANSIENT * transient;
}

static void show_win(wm_t* wm, win_t* win) {
	if (!win) {
		return;
	}

	LOG_VERBOSE("%p: Show window %p", wm, win)

	win->pixmap_modified = 1;

	// get the window type (with the _NET_WM_WINDOW_TYPE atom)

	xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_window_type(&wm->root->ewmh, win->win);
	xcb_ewmh_get_atoms_reply_t reply;

	if (xcb_ewmh_get_wm_window_type_reply(&wm->root->ewmh, cookie, &reply, NULL)) {
		while (reply.atoms_len --> 0) {
			xcb_atom_t atom = reply.atoms[reply.atoms_len];
			__set_type(wm, win, atom);
		}

		xcb_ewmh_get_atoms_reply_wipe(&reply);
	}

	win->visible = 1;
	call_cb(wm, win, CB_SHOW);
}

static void hide_win(wm_t* wm, win_t* win) {
	if (!win) {
		return;
	}

	LOG_VERBOSE("%p: Hide window %p", wm, win)

	win->visible = 0;
	call_cb(wm, win, CB_HIDE);
}

static void modify_win(wm_t* wm, win_t* win, unsigned resize) {
	if (!win) {
		return;
	}

	LOG_VERBOSE("%p: %s window %p (%dx%d+%d+%d)", wm, resize ? "Resize" : "Reposition", win, win->x_res, win->y_res, win->x_pos, win->y_pos)

	if (resize) {
		win->pixmap_modified = 1;
	}

	call_cb(wm, win, CB_MODIFY);
}

static void rem_win(wm_t* wm, win_t* win) {
	if (!win) {
		return;
	}

	LOG_VERBOSE("%p: Remove window %p from linked list", wm, win)

	// was the window at the head or tail of the parent?
	// if not, stitch the previous/next window to the next/previous window in the list

	if (wm->win_tail == win) {
		wm->win_tail = win->prev;
	}

	else {
		win->next->prev = win->prev;
	}

	if (wm->win_head == win) {
		wm->win_head = win->next;
	}

	else {
		win->prev->next = win->next;
	}

	// finally, free the window object itself

	call_cb(wm, win, CB_DELETE);
	free(win);

	wm->win_count--;
	update_client_list(wm);
}

static void focus_win(wm_t* wm, win_t* win) {
	if (!win) {
		return;
	}

	LOG_VERBOSE("%p: Focus window %p", wm, win)

	call_cb(wm, win, CB_FOCUS);
}

static int click_intended_for_us(wm_t* wm, xcb_button_press_event_t* detail) {
	// was the click intended for us or should we pass it on to the window?

	if (detail->state & SUPER_MOD) {
		return 1; // always intended for us if the super key is held
	}

	// check if the app has registered a CB_CLICK callback with special rules
	// we can't use the regular 'call_cb' function here because this callback takes in pointer XY coordinated instead of a window pointer for arguments

	uint64_t cb = wm->cbs[CB_CLICK];
	uint64_t param = wm->cb_params[CB_CLICK];

	if (!cb) {
		return 0; // don't assume the click was intended for us if no callback
	}

	float x =       (float) detail->root_x / wm->root->x_res;
	float y = 1.0 - (float) detail->root_y / wm->root->y_res;

	// TODO 4 arguments is too much for a KOS callback (can probably remove the wm parameter; it's a bit redundant)
	// return kos_callback(cb, 4, (uint64_t) wm, *(uint64_t*) &x, *(uint64_t*) &y, param);

	uint64_t ux = ((union { float _; uint64_t u; }) x).u;
	uint64_t uy = ((union { float _; uint64_t u; }) y).u;

	return kos_callback(cb, 3, ux, uy, param);
}

static void _state_mod(wm_t* wm, win_t* win, aquabsd_alps_win_state_t state, unsigned action) {
	unsigned value = win->state & state;

	if (action == XCB_EWMH_WM_STATE_ADD) {
		value = 1;
	}

	else if (action == XCB_EWMH_WM_STATE_REMOVE) {
		value = 0;
	}

	else if (action == XCB_EWMH_WM_STATE_TOGGLE) {
		value = !value;
	}

	if (!!(win->state & state) == value) {
		return;
	}

	LOG_VERBOSE("%p: Modify state of window %p (state 0x%x to be %s)", wm, win, state, value ? "set" : "unset")

	win->state &= ~state;
	win->state |= value * state;

	// update EWMH state

	LOG_VERBOSE("%p: Reflect new state through the EWMH atoms")

	uint32_t len = 0;
	xcb_atom_t atoms[16 /* provision */];

	#define ADD_STATE_ATOM(atom_name) \
		if (win->state & AQUABSD_ALPS_WIN_STATE_##atom_name) { \
			atoms[len++] = wm->root->ewmh._NET_WM_STATE_##atom_name; \
		}

	ADD_STATE_ATOM(FULLSCREEN)

	xcb_ewmh_set_wm_state(&wm->root->ewmh, win->win, len, atoms);
}

static inline int __process_state(wm_t* wm, win_t* win, xcb_client_message_event_t* detail) {
	if (!win) {
		return -1;
	}

	uint32_t action = detail->data.data32[0];
	uint32_t type = detail->data.data32[1];

	if (type == wm->root->ewmh._NET_WM_STATE_FULLSCREEN) {
		_state_mod(wm, win, AQUABSD_ALPS_WIN_STATE_FULLSCREEN, action);
	}

	else {
		LOG_WARN("State change requested of an unknown type (%d)", type)
		return -1; // not a type we know of
	}

	return call_cb(wm, win, CB_STATE);
}

static inline int __process_caption(wm_t* wm, win_t* win) {
	if (!win) {
		return -1;
	}

	// don't get caption for displaying in the log, as this is not the window manager device's responsibility

	LOG_VERBOSE("%p: Caption of window %p changed", wm, win)

	// frankly not much to do here lol

	return call_cb(wm, win, CB_CAPTION);
}

static inline int __process_dwd(wm_t* wm, win_t* win) {
	if (!win) {
		return -1;
	}

	// still not the window manager device's responsibility to log

	LOG_VERBOSE("%p: AQUA DWD state of %p changed", wm, win)

	// again, not much to do here

	return call_cb(wm, win, CB_DWD);
}

#define WIN_CONFIG \
	if (win) { \
		win->x_pos = detail->x; \
		win->y_pos = detail->y; \
		\
		win->x_res = detail->width; \
		win->y_res = detail->height; \
		\
		win->wm_x_res = wm->root->x_res; \
		win->wm_y_res = wm->root->y_res; \
	}

static int process_event(void* _wm, int type, xcb_generic_event_t* event) {
	// TODO see if the fact that the event loop may start before the WM/compositor is setup poses a problem

	wm_t* wm = _wm;

	// window management events
	// these are in a way "requests" by clients to perform certain actions (change their size, hide themselves, &c)
	// all of the definitions of these structs can be found in <xcb/xproto.h>

	if (type == XCB_CREATE_NOTIFY) {
		xcb_create_notify_event_t* detail = (void*) event;

		if (detail->window == wm->root->auxiliary) {
			return 0;
		}

		win_t* win = add_win(wm, detail->window);
		WIN_CONFIG
	}

	else if (type == XCB_DESTROY_NOTIFY) {
		xcb_destroy_notify_event_t* detail = (void*) event;
		rem_win(wm, search_win(wm, detail->window));
	}

	else if (type == XCB_MAP_NOTIFY) { // show window
		xcb_map_notify_event_t* detail = (void*) event;
		show_win(wm, search_win(wm, detail->window));

		// make it so that we receive this window's mouse events too
		// this is saying we want focus change and button events from the window

		uint32_t const attribs[] = {
			XCB_EVENT_MASK_FOCUS_CHANGE |
			XCB_EVENT_MASK_PROPERTY_CHANGE
		};

		xcb_change_window_attributes(wm->root->connection, detail->window, XCB_CW_EVENT_MASK, attribs);
		xcb_grab_button(wm->root->connection, 1, detail->window, XCB_EVENT_MASK_BUTTON_PRESS, XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_WINDOW_NONE, XCB_CURSOR_NONE, XCB_BUTTON_INDEX_ANY, XCB_BUTTON_MASK_ANY);
	}

	else if (type == XCB_UNMAP_NOTIFY) { // hide window
		xcb_unmap_notify_event_t* detail = (void*) event;
		hide_win(wm, search_win(wm, detail->window));
	}

	else if (type == XCB_CONFIGURE_NOTIFY) { // move/resize window
		xcb_configure_notify_event_t* detail = (void*) event;
		win_t* win = search_win(wm, detail->window);

		if (win) {
			unsigned resize =
				win->x_res != detail->width ||
				win->y_res != detail->height;

			WIN_CONFIG
			modify_win(wm, win, resize);
		}
	}

	else if (type == XCB_FOCUS_IN) { // focus window
		// do NOTHING
		// I don't know WHY THE HELL THIS HAPPENS, but KEY RELEASES SPECIFICALLY FOCUS WINDOWS FOR NO REASON
		// AAAAAAAAAAAAAGGGGGGGGGGGHHHHHH!!!!!!!!!!!!!

		// xcb_focus_in_event_t* detail = (void*) event;
		// focus_win(wm, search_win(wm, detail->event));
	}

	// mouse events

	else if (type == XCB_MOTION_NOTIFY) {
		xcb_motion_notify_event_t* detail = (void*) event;

		wm->root->mouse_x = detail->root_x;
		wm->root->mouse_y = detail->root_y;
	}

	else if (type == XCB_BUTTON_PRESS) {
		xcb_button_press_event_t* detail = (void*) event;

		wm->root->mouse_x = detail->root_x;
		wm->root->mouse_y = detail->root_y;

		// basically, as far as I understand it, the WM must decide if a button press is for itself of a client over here
		// if it's for the client (e.g. we clicked on the contents of the window), no problem, just replay the pointer event:
		// xcb_allow_events(wm->root->connection, XCB_ALLOW_REPLAY_POINTER, detail->time);
		// otherwise, things get a little more complicated; we must then grab the pointer:
		// xcb_grab_pointer(wm->root->connection, 1, wm->root->win, event_mask, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_CURRENT_TIME);
		// (both pointer and keyboard must be asynchronous)
		// then, until we receive an XCB_BUTTON_RELEASE event, we can process the mouse events as if they were ours
		// once XCB_BUTTON_RELEASE is received though, we need to ungrab the pointer:
		// xcb_ungrab_pointer(wm->root->connection);
		// so that operations can resume normally

		// TODO check out xcb_drag (https://github.com/i3/i3/issues/3086)

		wm->in_wm_click = click_intended_for_us(wm, detail);

		if (wm->in_wm_click) {
			// prevent event from passing through to the client

			uint32_t event_mask =
				XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
				XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION;

			xcb_grab_pointer(wm->root->connection, 1, wm->root->win, event_mask, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_CURRENT_TIME);
			xcb_grab_button(wm->root->connection, 1, wm->root->win, event_mask, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_BUTTON_INDEX_ANY, SUPER_MOD);
		}

		else {
			// focus the window
			// TODO what's the difference between 'xcb_button_press_event_t.event' & 'xcb_button_press_event_t.child'?

			if (!wm->root->mouse_axes[AQUABSD_ALPS_MOUSE_AXIS_Z]) { // we don't wanna focus when sending a scrollwheel event
				win_t* win = search_win(wm, detail->event);

				if (win) {
					aquabsd_alps_win_grab_focus(win);
					focus_win(wm, win);
				}
			}

			// allow mouse click to go through to window

			xcb_allow_events(wm->root->connection, XCB_ALLOW_REPLAY_POINTER, detail->time);

			// cancel any processed mouse events

			memset(wm->root->mouse_buttons, 0, sizeof wm->root->mouse_buttons);
			memset(wm->root->mouse_axes,    0, sizeof wm->root->mouse_axes);
		}

		xcb_flush(wm->root->connection);
	}

	else if (type == XCB_BUTTON_RELEASE) {
		xcb_button_release_event_t* detail = (void*) event;

		wm->root->mouse_x = detail->root_x;
		wm->root->mouse_y = detail->root_y;

		// honestly don't ask me how/why this stuff works
		// this is the fruit of essentially just randomly scrambling the following lines for a few hours to see what works
		// there's no logic behind this, pure absurdity

		xcb_ungrab_pointer(wm->root->connection, XCB_CURRENT_TIME);

		if (wm->in_wm_click) {
			wm->in_wm_click = 0;

			xcb_ungrab_button(wm->root->connection, XCB_BUTTON_INDEX_ANY, wm->root->win, XCB_GRAB_ANY);

			uint32_t event_mask =
				XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
				XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION;

			xcb_grab_button(wm->root->connection, 1, wm->root->win, event_mask, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_BUTTON_INDEX_ANY, SUPER_MOD);
		}

		else {
			// allow mouse release to go through to window

			xcb_allow_events(wm->root->connection, XCB_ALLOW_REPLAY_POINTER, detail->time);

			// cancel any processed mouse events

			memset(wm->root->mouse_buttons, 0, sizeof wm->root->mouse_buttons);
			memset(wm->root->mouse_axes,    0, sizeof wm->root->mouse_axes);
		}

		xcb_flush(wm->root->connection);
	}

	else if (type == XCB_CLIENT_MESSAGE) {
		xcb_client_message_event_t* detail = (void*) event;
		win_t* win = search_win(wm, detail->window);

		if (win && detail->type == wm->root->ewmh._NET_WM_STATE) {
			__process_state(wm, win, detail);
		}
	}

	else if (type == XCB_PROPERTY_NOTIFY) {
		xcb_property_notify_event_t* detail = (void*) event;
		win_t* win = search_win(wm, detail->window);

		// TODO check out if we really want to process on each value of detail->state

		xcb_atom_t atom = detail->atom;

		if (win && (
			atom == XCB_ATOM_WM_NAME ||
			atom == wm->root->ewmh._NET_WM_NAME ||
			atom == wm->root->ewmh._NET_WM_VISIBLE_NAME
		)) {
			__process_caption(wm, win);
		}

		else if (win && (
			atom == wm->root->dwd_supports_atom ||
			atom == wm->root->dwd_close_pos_atom
		)) {
			__process_dwd(wm, win);
		}
	}

	return 0;
}

static void predraw(void* _wm) {
	wm_t* wm = _wm;

	// just query the pointer position directly
	// I've spend fucking hours trying to get this work "correctly", but every SO answer or demo works fine on its own, but isn't applicable whatsoever here
	// how anyone ever made anything functional with X11 is a mystery to me
	// how anyone thought it was a good idea to stick with an outdated and shitty protocol when the need for compositors on desktops arose instead of building Wayland earlier is a mystery to me
	// I long for the day when I can piss on X11's grave
	// traditionally, this'd be done with 'xcb_query_pointer', but since we anyway need to get the cursor image, we can kill two birds with one stone
	// get the cursor image (using Xfixes extension)

	xcb_xfixes_get_cursor_image_and_name_cookie_t cookie = xcb_xfixes_get_cursor_image_and_name(wm->root->connection);
	xcb_xfixes_get_cursor_image_and_name_reply_t* reply = xcb_xfixes_get_cursor_image_and_name_reply(wm->root->connection, cookie, NULL);

	wm->root->mouse_x = reply->x;
	wm->root->mouse_y = reply->y;

	int len = xcb_xfixes_get_cursor_image_and_name_name_length(reply);
	char* name = xcb_xfixes_get_cursor_image_and_name_name(reply);

	uint16_t width  = reply->width;
	uint16_t height = reply->height;

	uint16_t xhot = reply->xhot;
	uint16_t yhot = reply->yhot;

	wm->cursor_xhot = (float) xhot / width  * 2 - 1;
	wm->cursor_yhot = (float) yhot / height * 2 - 1;

	char* dash = memrchr(name, '-', len);

	size_t len_before_dash = dash - name;
	size_t len_after_dash = len - len_before_dash;

	// these cursor names come either from the X11 or FreeDesktop cursor specs:
	// X11: https://www.x.org/releases/current/doc/libX11/libX11/libX11.html#x_font_cursors
	// X11: https://tronche.com/gui/x/xlib/appendix/b/
	// FreeDesktop: https://www.freedesktop.org/wiki/Specifications/cursor-spec/

	wm->cursor = "regular"; // default

	if (!len) {
		if (!xhot && !yhot) {
			// TODO not necessarily hidden, could be a custom cursor, but too lazy to handle that case for now
			wm->cursor = "hidden";
		}

		else {
			wm->cursor = "regular";
		}
	}

	else if (!strncmp(name, "left_ptr", len)) {
		wm->cursor = "regular";
	}

	else if (
		!strncmp(name, "xterm", len) ||
		!strncmp(name, "text", len)
	) {
		wm->cursor = "caret";
	}

	else if (
		!strncmp(name, "hand1", len) ||
		!strncmp(name, "grab", len)
	) {
		wm->cursor = "hand";
	}

	else if (
		!strncmp(name, "dnd-none", len) ||
		!strncmp(name, "grabbing", len)
	) {
		wm->cursor = "drag";
	}

	else if (
		!strncmp(name, "pointer", len) ||
		!strncmp(name, "hand2", len)
	) {
		wm->cursor = "pointer";
	}

	else if (!strncmp(name, "help", len)) {
		wm->cursor = "question";
	}

	else if (dash && !strncmp(dash, "-resize", len_after_dash)) {
		// other directions

		if (!strncmp(name, "col", 3)) {
			wm->cursor = "resize-l";
		}

		else if (!strncmp(name, "row", 3)) {
			wm->cursor = "resize-t";
		}

		// two directions (important this comes first because of 'strncmp')

		else if (!strncmp(name, "ne", 2)) {
			wm->cursor = "resize-tr";
		}

		else if (!strncmp(name, "se", 2)) { // SOUF EAST LUNDUN INNIT
			wm->cursor = "resize-br";
		}

		else if (!strncmp(name, "sw", 2)) {
			wm->cursor = "resize-bl";
		}

		else if (!strncmp(name, "nw", 2)) {
			wm->cursor = "resize-tl";
		}

		// one direction 🎶

		else if (*name == 'n') {
			wm->cursor = "resize-t";
		}

		else if (*name == 'e') {
			wm->cursor = "resize-r";
		}

		else if (*name == 's') {
			wm->cursor = "resize-b";
		}

		else if (*name == 'w') {
			wm->cursor = "resize-l";
		}

		// as the french would put it: ppt

		else {
			LOG_WARN("%p: Unknown resize cursor direction \"%.*s\"", wm, len, name)
		}
	}

	else {
		LOG_WARN("%p: Unknown cursor type \"%.*s\" (w = %d, h = %d, xhot = %d, yhot = %d)", wm, len, name, width, height, xhot, yhot)
	}

	free(reply);
	return;
}

dynamic wm_t* create(void) {
	LOG_INFO("Create window manager")

	wm_t* wm = calloc(1, sizeof *wm);

	// create a window with the help of the 'aquabsd.alps.win' device

	LOG_VERBOSE("Set root window up")

	wm->root = aquabsd_alps_win_create_setup();

	if (!wm->root) {
		FATAL_ERROR("Failed to setup root window")
	}

	wm->root->win = wm->root->screen->root;

	// get width & height of root window

	wm->root->x_res = wm->root->wm_x_res;
	wm->root->y_res = wm->root->wm_y_res;

	// TODO make this comment correct
	// tell X to send us all 'CreateNotify', 'ConfigureNotify', and 'DestroyNotify' events ('SubstructureNotifyMask' also sends back some other events but we're not using those)

	LOG_VERBOSE("Set event mask")

	const uint32_t attribs[] = {
		// XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_STRUCTURE_NOTIFY |
		XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
		XCB_EVENT_MASK_EXPOSURE |
		XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
		XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION |
		XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
		XCB_EVENT_MASK_PROPERTY_CHANGE
	};

	xcb_change_window_attributes(wm->root->connection, wm->root->win, XCB_CW_EVENT_MASK, attribs);

	// grab the pointer

	LOG_VERBOSE("Grab pointer")

	// xcb_grab_button(wm->root->connection, 1, wm->root->win, XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_BUTTON_INDEX_ANY, XCB_BUTTON_MASK_ANY);
	// xcb_ungrab_button(wm->root->connection, XCB_BUTTON_INDEX_ANY, wm->root->win, XCB_GRAB_ANY);

	uint32_t event_mask =
		XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
		XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION;

	xcb_grab_button(wm->root->connection, 1, wm->root->win, event_mask, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_BUTTON_INDEX_ANY, SUPER_MOD);

	// grab the keys we are interested in as a window manager (i.e. those with the super key modifier)

	LOG_VERBOSE("Grab keys we're interested in (i.e. Super+*)")

	xcb_ungrab_key(wm->root->connection, XCB_GRAB_ANY, wm->root->win, XCB_MOD_MASK_ANY);

	xcb_grab_key(wm->root->connection, 1, wm->root->win, SUPER_MOD, XCB_GRAB_ANY,     XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
	xcb_grab_key(wm->root->connection, 1, wm->root->win, XCB_MOD_MASK_ANY, SUPER_KEY, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

	// now, we move on to '_NET_SUPPORTING_WM_CHECK'
	// this is a bit weird, but it's all specified by the EWMH spec: https://developer.gnome.org/wm-spec/

	LOG_VERBOSE("Create support window for _NET_SUPPORTING_WM_CHECK (required to comply with the EWMH specification)")

	wm->support_win = create_dumb_win(wm);
	xcb_window_t support_win_list[1] = { wm->support_win };

	xcb_change_property(wm->root->connection, XCB_PROP_MODE_REPLACE, wm->root->win, wm->root->ewmh._NET_SUPPORTING_WM_CHECK, XCB_ATOM_WINDOW, 32, 1, support_win_list);
	xcb_change_property(wm->root->connection, XCB_PROP_MODE_REPLACE, wm->support_win, wm->root->ewmh._NET_SUPPORTING_WM_CHECK, XCB_ATOM_WINDOW, 32, 1, support_win_list);

	// specify which atoms are supported in '_NET_SUPPORTED'

	LOG_VERBOSE("Specify which atoms we support with _NET_SUPPORTED")

	const xcb_atom_t supported_atoms[] = {
		wm->root->ewmh._NET_SUPPORTED,
		wm->root->ewmh._NET_SUPPORTING_WM_CHECK,
		wm->root->ewmh._NET_CLIENT_LIST,

		wm->root->ewmh._NET_WM_PID,

		wm->root->ewmh._NET_WM_NAME,
		wm->root->ewmh._NET_WM_VISIBLE_NAME,

		wm->root->ewmh._NET_WM_STATE,
		wm->root->ewmh._NET_WM_STATE_FULLSCREEN,

		wm->root->ewmh._NET_WM_WINDOW_TYPE,
		wm->root->ewmh._NET_WM_WINDOW_TYPE_DESKTOP,
		wm->root->ewmh._NET_WM_WINDOW_TYPE_DOCK,
		wm->root->ewmh._NET_WM_WINDOW_TYPE_TOOLBAR,
		wm->root->ewmh._NET_WM_WINDOW_TYPE_MENU,
		wm->root->ewmh._NET_WM_WINDOW_TYPE_UTILITY,
		wm->root->ewmh._NET_WM_WINDOW_TYPE_SPLASH,
		wm->root->ewmh._NET_WM_WINDOW_TYPE_DIALOG,
		wm->root->ewmh._NET_WM_WINDOW_TYPE_NORMAL,
		wm->root->ewmh._NET_WM_WINDOW_TYPE_NOTIFICATION,
	};

	xcb_change_property(wm->root->connection, XCB_PROP_MODE_REPLACE, wm->root->win, wm->root->ewmh._NET_SUPPORTED, XCB_ATOM_ATOM, 32, sizeof(supported_atoms) / sizeof(*supported_atoms), supported_atoms);

	// get all monitors and their geometries
	// CRTC stands for "Cathode Ray Tube Controller", which is a dumb name, so AQUA calls them "providers"

	LOG_VERBOSE("Get all monitors and their resolutions (providers in AQUA lingo, CRTC in X11 lingo)")

	xcb_randr_get_screen_resources_cookie_t res_cookie = xcb_randr_get_screen_resources(wm->root->connection, wm->root->win);
	xcb_randr_get_screen_resources_reply_t* res = xcb_randr_get_screen_resources_reply(wm->root->connection, res_cookie, NULL);

	if (!res) {
		LOG_WARN("Couldn't get a list of providers; create single virtual provider to cover the whole root window")

		// too bad 😥
		// if we can't get a list of all providers, at least create one "virtual" provider which covers the root window

		const provider_t VIRTUAL_PROVIDER = {
			.x = 0,
			.y = 0,

			.x_res = wm->root->x_res,
			.y_res = wm->root->y_res,
		};

		wm->providers = malloc(sizeof *wm->providers);
		memcpy(&wm->providers[0], &VIRTUAL_PROVIDER, sizeof VIRTUAL_PROVIDER);

		goto skip_geom;
	}

	int provider_count = xcb_randr_get_screen_resources_crtcs_length(res);
	xcb_randr_crtc_t* providers = xcb_randr_get_screen_resources_crtcs(res);

	for (int i = 0; i < provider_count; i++) {
		xcb_randr_get_crtc_info_cookie_t info_cookie = xcb_randr_get_crtc_info(wm->root->connection, providers[i], 0);
		xcb_randr_get_crtc_info_reply_t* info = xcb_randr_get_crtc_info_reply(wm->root->connection, info_cookie, 0);

		if (!info->width || !info->height) { // ??? reports 4 providers when there are only two so discard the bs ones
			LOG_WARN("Provider %d is bogus", i)
			goto skip;
		}

		wm->providers = realloc(wm->providers, ++wm->provider_count * sizeof *wm->providers);
		provider_t* provider = &wm->providers[wm->provider_count - 1];

		provider->x = info->x;
		provider->y = info->y;

		provider->x_res = info->width;
		provider->y_res = info->height;

		LOG_INFO("Provider %d: %dx%d+%d+%d", i, provider->x_res, provider->y_res, provider->x, provider->y)

	skip:

		free(info);
	}

	free(res);

skip_geom:

	// flush

	xcb_flush(wm->root->connection);

	// set the fields reserved to us in the root window object

	wm->root->wm_object = wm;

	wm->root->wm_event_cb = process_event;
	wm->root->wm_predraw_cb = predraw;

	LOG_SUCCESS("Window manager created: %p", wm)

	return wm;
}

dynamic win_t* get_root_win(wm_t* wm) {
	return wm->root;
}

// TODO should these functions be in aquabsd.alps.win?
//      then, the client can call 'get_root_win' to access the window object and call 'get_x/y_res' on that instead

dynamic unsigned get_x_res(wm_t* wm) {
	return wm->root->x_res;
}

dynamic unsigned get_y_res(wm_t* wm) {
	return wm->root->y_res;
}

dynamic char* get_cursor(wm_t* wm) {
	return wm->cursor;
}

dynamic float get_cursor_xhot(wm_t* wm) {
	return wm->cursor_xhot;
}

dynamic float get_cursor_yhot(wm_t* wm) {
	return wm->cursor_yhot;
}

dynamic int set_name(wm_t* wm, const char* name) {
	LOG_VERBOSE("%p: Set name to \"%s\"", wm, name)

	win_t support_win = { // shim for 'aquabsd_alps_win_set_caption'
		.connection = wm->root->connection,
		.win = wm->support_win
	};

	aquabsd_alps_win_set_caption(&support_win, name);

	return 0;
}

dynamic int register_cb(wm_t* wm, cb_t type, uint64_t cb, uint64_t param) {
	if (type >= CB_LEN) {
		LOG_WARN("Callback type %d doesn't exist", type)
		return -1;
	}

	wm->cbs[type] = cb;
	wm->cb_params[type] = param;

	return 0;
}

// this function is only for compositing window managers

dynamic int make_compositing(wm_t* wm) {
	LOG_INFO("%p: Make window manager compositing", wm)

	// make it so that our compositing window manager can be recognized as such by other processes

	LOG_VERBOSE("%p: Make window manager recognizable to other processes as compositing")

	xcb_window_t screen_owner = create_dumb_win(wm);
	xcb_change_property(wm->root->connection, XCB_PROP_MODE_REPLACE, screen_owner, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen("xcompmgr") /* don't need to include null */, "xcompmgr");

	if (wm->root->default_screen > 99) {
		LOG_FATAL("%p: Root window's default screen (%d) is greater than 99", wm->root->default_screen)
		return -1;
	}

	xcb_atom_t atom = wm->root->ewmh._NET_WM_CM_Sn[wm->root->default_screen];
	xcb_set_selection_owner(wm->root->connection, screen_owner, atom, 0);

	// we want to enable manual redirection, because we want to track damage and flush updates ourselves
	// if we were to pass 'XCB_COMPOSITE_REDIRECT_AUTOMATIC' instead, the server would handle all that internally

	LOG_VERBOSE("%p: Enable manual redirection", wm)

	xcb_composite_redirect_subwindows(wm->root->connection, wm->root->win, XCB_COMPOSITE_REDIRECT_MANUAL);

	// get the overlay window
	// this window allows us to draw what we want on a layer between normal windows and the screensaver without interference
	// basically, it's as if there was a giant screen-filling window above all the others
	// if we were to attach an OpenGL context to it with 'aquabsd.alps.ogl', we could render anything we want to it
	// that includes the underlying windows
	// (in that case we would do that by getting an OpenGL texture with their contents through 'CMD_BIND_WIN_TEX')

	LOG_VERBOSE("%p: Get overlay window", wm)

	xcb_composite_get_overlay_window_cookie_t overlay_win_cookie = xcb_composite_get_overlay_window(wm->root->connection, wm->root->win);
	xcb_composite_get_overlay_window_reply_t* overlay_win_reply = xcb_composite_get_overlay_window_reply(wm->root->connection, overlay_win_cookie, NULL);

	xcb_window_t overlay_win = overlay_win_reply->overlay_win;
	free(overlay_win_reply);

	// for whatever reason, X (and even Xcomposite, which is even weirder) doesn't include a way to make windows transparent to events
	// so we must use the Xfixes extension to make the overlay window transparent to events and pass them on through to lower windows
	// remember, the overlay is like one giant window above all our other windows
	// why/how does this work? I don't know and no one seems to know either, this was stolen from picom, and I can't remember how I figured out the Xlib equivalent bit of code in x-compositing-wm
	// also, we must "negotiate the version" of the Xfixes extension (i.e. just initialize) before we use it, or we get UB

	LOG_VERBOSE("Negotiate version of the Xfixes extension")

	xcb_xfixes_query_version(wm->root->connection, XCB_XFIXES_MAJOR_VERSION, XCB_XFIXES_MINOR_VERSION);

	LOG_VERBOSE("Make overlay window transparent to events (I don't know why I have to do this)")

	xcb_shape_mask(wm->root->connection, XCB_SHAPE_SO_SET, XCB_SHAPE_SK_BOUNDING, overlay_win, 0, 0, 0);
	xcb_shape_rectangles(wm->root->connection, XCB_SHAPE_SO_SET, XCB_SHAPE_SK_INPUT, XCB_CLIP_ORDERING_UNSORTED, overlay_win, 0, 0, 0, NULL);

	// phew! now, we simply set the auxiliary window (the draw window) of the root to our overlay
	// this window shouldn't be used for events or anything of the sorts, only for drawing, which is why it mustn't replace the primary window
	// use the root window for events instead

	wm->root->auxiliary = overlay_win;

	LOG_SUCCESS("%p: Made window manager compositing successfully", wm)

	return 0;
}

// provider stuff

dynamic int provider_count(wm_t* wm) {
	return wm->provider_count;
}

dynamic int provider_x(wm_t* wm, unsigned id) {
	return wm->providers[id].x;
}

dynamic int provider_y(wm_t* wm, unsigned id) {
	return wm->providers[id].y;
}

dynamic int provider_x_res(wm_t* wm, unsigned id) {
	return wm->providers[id].x_res;
}

dynamic int provider_y_res(wm_t* wm, unsigned id) {
	return wm->providers[id].y_res;
}
