#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUPER_MOD XCB_MOD_MASK_4 // looks like this is the super key

#define FATAL_ERROR(...) fprintf(stderr, "[aquabsd.alps.wm] FATAL ERROR "__VA_ARGS__); delete(wm); return NULL;
#define WARNING(...) fprintf(stderr, "[aquabsd.alps.wm] WARNING "__VA_ARGS__);

// helper functions (for XCB)

static inline xcb_atom_t get_intern_atom(wm_t* wm, const char* name) {
	// TODO obviously, this function isn't super ideal for leveraging the benefits XCB provides over Xlib
	//      at some point, refactor this so that... well all this work converting from Xlib to XCB isn't for nothing

	xcb_intern_atom_cookie_t atom_cookie = xcb_intern_atom(wm->root->connection, 0, strlen(name), name);
	return xcb_intern_atom_reply(wm->root->connection, atom_cookie, NULL)->atom;
}

static inline xcb_window_t create_dumb_win(wm_t* wm) {
	xcb_window_t win = xcb_generate_id(wm->root->connection);
	xcb_create_window(wm->root->connection, XCB_COPY_FROM_PARENT, win, wm->root->win, 0, 0, 1, 1, 0, XCB_WINDOW_CLASS_COPY_FROM_PARENT, 0, 0, NULL);

	return win;
}

// actual functions

dynamic int delete(wm_t* wm) {
	if (wm->root) {
		aquabsd_alps_win_delete(wm->root);
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
	xcb_window_t* client_list = calloc(wm->win_count, sizeof *client_list);
	unsigned i = 0;

	for (win_t* win = wm->win_head; win; win = win->next) {
		client_list[i++] = win->win;
	}

	xcb_change_property(wm->root->connection, XCB_PROP_MODE_REPLACE, wm->root->win, wm->client_list_atom, XCB_ATOM_WINDOW, 32, wm->win_count, client_list);
	xcb_flush(wm->root->connection);

	free(client_list);
}

static win_t* add_win(wm_t* wm, xcb_window_t id) {
	win_t* win = calloc(1, sizeof *win);

	win->connection = wm->root->connection;
	win->win = id;

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
	// in a compositing window manager, this means we're dealing with the overlay
	// prevent that

	if (id == wm->root->auxiliary) {
		return NULL;
	}

	win_t* win = NULL;

	for (win = wm->win_head; win; win = win->next) {
		if (win->win == id) {
			return win;
		}
	}

	if (!win) {
		fprintf(stderr, "[aquabsd.alps.wm] WARNING Window of id 0x%x was not found\n", id);
	}

	return NULL; // something's probably gonna crash ;)
}

static void show_win(wm_t* wm, win_t* win) {
	if (!win) {
		return;
	}

	win->visible = 1;
	call_cb(wm, win, CB_SHOW);
}

static void hide_win(wm_t* wm, win_t* win) {
	if (!win) {
		return;
	}

	win->visible = 0;
	call_cb(wm, win, CB_HIDE);
}

static void modify_win(wm_t* wm, win_t* win) {
	if (!win) {
		return;
	}

	win->pixmap_modified = 1;
	call_cb(wm, win, CB_MODIFY);
}

static void rem_win(wm_t* wm, win_t* win) {
	if (!win) {
		return;
	}

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

	free(win);
	call_cb(wm, win, CB_DELETE);

	wm->win_count--;
	update_client_list(wm);
}

static void focus_win(wm_t* wm, win_t* win) {
	if (!win) {
		return;
	}

	call_cb(wm, win, CB_FOCUS);
}

#define WIN_CONFIG \
	win->x_pos = detail->x; \
	win->y_pos = detail->y; \
	\
	win->x_res = detail->width; \
	win->y_res = detail->height; \
	\
	win->wm_x_res = wm->root->x_res; \
	win->wm_y_res = wm->root->y_res;

static int process_event(void* _wm, int type, xcb_generic_event_t* event) {
	wm_t* wm = _wm;

	// window management events
	// all of the definitions of these structs can be found in <xcb/xproto.h>

	if (type == XCB_CREATE_NOTIFY) {
		xcb_create_notify_event_t* detail = (void*) event;

		if (detail->window == wm->root->auxiliary) {
			return 0;
		}

		// make it so that we receive this window's mouse events too
		// this is saying we want focus change and button events from the window

		const uint32_t attribs[] = {
			XCB_EVENT_MASK_FOCUS_CHANGE
		};

		xcb_change_window_attributes(wm->root->connection, detail->window, XCB_CW_EVENT_MASK, attribs);
		xcb_grab_button(wm->root->connection, 1, detail->window, XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION, XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_BUTTON_INDEX_ANY, XCB_MOD_MASK_ANY);

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
	}

	else if (type == XCB_UNMAP_NOTIFY) { // hide window
		xcb_unmap_notify_event_t* detail = (void*) event;
		hide_win(wm, search_win(wm, detail->window));
	}

	else if (type == XCB_CONFIGURE_NOTIFY) { // move/resize window
		xcb_configure_notify_event_t* detail = (void*) event;
		win_t* win = search_win(wm, detail->window);

		WIN_CONFIG
		modify_win(wm, win);
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

		// TODO basically, as far as I understand it, the WM must decide if a button press is for itself of a client over here
		//      if it's for the client (e.g. we clicked on the contents of the window), no problem, just replay the pointer event:
		//      xcb_allow_events(wm->root->connection, XCB_ALLOW_REPLAY_POINTER, detail->time);
		//      otherwise, things get a little more complicated; we must then grab the pointer:
		//      xcb_grab_pointer(wm->root->connection, 1, wm->root->win, XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_CURRENT_TIME);
		//      (both pointer and keyboard must be asynchronous)
		//      then, until we receive an XCB_BUTTON_RELEASE event, we can process the mouse events as if they were ours
		//      once XCB_BUTTON_RELEASE is received though, we need to ungrab the pointer:
		//      xcb_ungrab_pointer(wm->root->connection);
		//      so that operations can resume normally

		if (detail->state & SUPER_MOD) {
			// prevent event from passing through to the client
			xcb_allow_events(wm->root->connection, XCB_ALLOW_SYNC_POINTER, detail->time);
		}

		else {
			// allow mouse click to go through to window

			xcb_allow_events(wm->root->connection, XCB_ALLOW_REPLAY_POINTER, detail->time);
			
			// focus the window
			// TODO what's the difference between 'xcb_button_press_event_t.event' & 'xcb_button_press_event_t.child'?

			if (!wm->root->mouse_axes[AQUABSD_ALPS_MOUSE_AXIS_Z]) { // we don't wanna focus when sending a scrollwheel event
				win_t* win = search_win(wm, detail->event);

				if (win) {
					aquabsd_alps_win_grab_focus(win);
					focus_win(wm, win);
				}
			}

			// cancel any processed mouse events

			memset(wm->root->mouse_buttons, 0, sizeof wm->root->mouse_buttons);
			memset(wm->root->mouse_axes,    0, sizeof wm->root->mouse_axes);
		}
	}

	else if (type == XCB_BUTTON_RELEASE) {
		xcb_button_release_event_t* detail = (void*) event;

		wm->root->mouse_x = detail->root_x;
		wm->root->mouse_y = detail->root_y;

		if (detail->state & SUPER_MOD) {
			// prevent event from passing through to the client
			xcb_allow_events(wm->root->connection, XCB_ALLOW_SYNC_POINTER, detail->time);
		}

		else {
			xcb_allow_events(wm->root->connection, XCB_ALLOW_REPLAY_POINTER, detail->time);

			// cancel any processed mouse events

			memset(wm->root->mouse_buttons, 0, sizeof wm->root->mouse_buttons);
			memset(wm->root->mouse_axes,    0, sizeof wm->root->mouse_axes);
		}
	}

	return 0;
}

dynamic wm_t* create(void) {
	wm_t* wm = calloc(1, sizeof *wm);

	// create a window with the help of the 'aquabsd.alps.win' device

	wm->root = aquabsd_alps_win_create_setup();

	if (!wm->root) {
		FATAL_ERROR("Failed to setup root window\n")
	}

	wm->root->win = wm->root->screen->root;

	// get width & height of root window

	wm->root->x_res = wm->root->wm_x_res;
	wm->root->y_res = wm->root->wm_y_res;

	// TODO make this comment correct
	// tell X to send us all 'CreateNotify', 'ConfigureNotify', and 'DestroyNotify' events ('SubstructureNotifyMask' also sends back some other events but we're not using those)

	const uint32_t attribs[] = {
		//XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_PROPERTY_CHANGE
		XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
		XCB_EVENT_MASK_EXPOSURE |
		XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION |
		XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE
	};

	xcb_change_window_attributes(wm->root->connection, wm->root->win, XCB_CW_EVENT_MASK, attribs);

	// grab the pointer

	// xcb_grab_pointer(wm->root->connection, 1, wm->root->win, XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_CURRENT_TIME);
	xcb_grab_button(wm->root->connection, 1, wm->root->win, XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_BUTTON_INDEX_ANY, SUPER_MOD);

	// grab the keys we are interested in as a window manager (i.e. those with the super key modifier)

	xcb_ungrab_key(wm->root->connection, XCB_GRAB_ANY, wm->root->win, XCB_MOD_MASK_ANY);
	xcb_grab_key(wm->root->connection, 1, wm->root->win, SUPER_MOD, XCB_GRAB_ANY, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

	// setup our atoms
	// we also need to specify which atoms are supported in '_NET_SUPPORTED'

	wm->client_list_atom = get_intern_atom(wm, "_NET_CLIENT_LIST");
	wm->supported_atoms_list_atom = get_intern_atom(wm, "_NET_SUPPORTED");

	aquabsd_alps_win_get_ewmh_atoms(wm->root);

	const xcb_atom_t supported_atoms[] = {
		wm->supported_atoms_list_atom,
		wm->client_list_atom,

		// from aquabsd.alps.win

		wm->root->_net_wm_visible_name_atom,
		wm->root->_net_wm_name_atom,
	};

	xcb_change_property(wm->root->connection, XCB_PROP_MODE_REPLACE, wm->root->win, wm->supported_atoms_list_atom, XCB_ATOM_ATOM, 32, sizeof(supported_atoms) / sizeof(*supported_atoms), supported_atoms);

	// now, we move on to '_NET_SUPPORTING_WM_CHECK'
	// this is a bit weird, but it's all specified by the EWMH spec: https://developer.gnome.org/wm-spec/

	xcb_atom_t supporting_wm_check_atom = get_intern_atom(wm, "_NET_SUPPORTING_WM_CHECK");
	wm->support_win = create_dumb_win(wm);

	xcb_window_t support_win_list[1] = { wm->support_win };

	xcb_change_property(wm->root->connection, XCB_PROP_MODE_REPLACE, wm->root->win, supporting_wm_check_atom, XCB_ATOM_WINDOW, 32, 1, support_win_list);
	xcb_change_property(wm->root->connection, XCB_PROP_MODE_REPLACE, wm->support_win, supporting_wm_check_atom, XCB_ATOM_WINDOW, 32, 1, support_win_list);

	// TODO get all monitors and their individual resolutions with Xinerama (or xrandr? idk whichever one's newer)

	// flush

	xcb_flush(wm->root->connection);

	// set the fields reserved to us in the root window object

	wm->root->wm_object = wm;
	wm->root->wm_event_cb = process_event;

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

dynamic int set_name(wm_t* wm, const char* name) {
	win_t support_win = { // shim for 'aquabsd_alps_win_set_caption'
		.connection = wm->root->connection,
		.win = wm->support_win
	};

	aquabsd_alps_win_set_caption(&support_win, name);

	return 0;
}

dynamic int register_cb(wm_t* wm, cb_t type, uint64_t cb, uint64_t param) {
	if (type >= CB_LEN) {
		fprintf(stderr, "[aquabsd.alps.wm] Callback type %d doesn't exist\n", type);
		return -1;
	}

	wm->cbs[type] = cb;
	wm->cb_params[type] = param;

	return 0;
}

// this function is only for compositing window managers

dynamic int make_compositing(wm_t* wm) {
	// make it so that our compositing window manager can be recognized as such by other processes

	xcb_window_t screen_owner = create_dumb_win(wm);
	xcb_change_property(wm->root->connection, XCB_PROP_MODE_REPLACE, screen_owner, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen("xcompmgr") /* don't need to include null */, "xcompmgr");

	char name[] = "_NET_WM_CM_S##";
	snprintf(name, sizeof(name), "_NET_WM_CM_S%d", wm->root->default_screen);

	xcb_atom_t atom = get_intern_atom(wm, name);
	xcb_set_selection_owner(wm->root->connection, screen_owner, atom, 0);

	// we want to enable manual redirection, because we want to track damage and flush updates ourselves
	// if we were to pass 'XCB_COMPOSITE_REDIRECT_AUTOMATIC' instead, the server would handle all that internally

	xcb_composite_redirect_subwindows(wm->root->connection, wm->root->win, XCB_COMPOSITE_REDIRECT_MANUAL);

	// get the overlay window
	// this window allows us to draw what we want on a layer between normal windows and the screensaver without interference
	// basically, it's as if there was a giant screen-filling window above all the others
	// if we were to attach an OpenGL context to it with 'aquabsd.alps.ogl', we could render anything we want to it
	// that includes the underlying windows
	// (in that case we would do that by getting an OpenGL texture with their contents through 'CMD_BIND_WIN_TEX')

	xcb_composite_get_overlay_window_cookie_t overlay_win_cookie = xcb_composite_get_overlay_window(wm->root->connection, wm->root->win);
	xcb_window_t overlay_win = xcb_composite_get_overlay_window_reply(wm->root->connection, overlay_win_cookie, NULL)->overlay_win;

	// for whatever reason, X (and even Xcomposite, which is even weirder) doesn't include a way to make windows transparent to events
	// so we must use the Xfixes extension to make the overlay window transparent to events and pass them on through to lower windows
	// remember, the overlay is like one giant window above all our other windows
	// why/how does this work? I don't know and no one seems to know either, this was stolen from picom, and I can't remember how I figured out the Xlib equivalent bit of code in x-compositing-wm
	// also, we must "negotiate the version" of the Xfixes extension (i.e. just initialize) before we use it, or we get UB

	xcb_xfixes_query_version(wm->root->connection, XCB_XFIXES_MAJOR_VERSION, XCB_XFIXES_MINOR_VERSION);

	xcb_shape_mask(wm->root->connection, XCB_SHAPE_SO_SET, XCB_SHAPE_SK_BOUNDING, overlay_win, 0, 0, 0);
	xcb_shape_rectangles(wm->root->connection, XCB_SHAPE_SO_SET, XCB_SHAPE_SK_INPUT, XCB_CLIP_ORDERING_UNSORTED, overlay_win, 0, 0, 0, NULL);

	// phew! now, we simply set the auxiliary window (the draw window) of the root to our overlay
	// this window shouldn't be used for events or anything of the sorts, only for drawing, which is why it mustn't replace the primary window
	// use the root window for events instead

	wm->root->auxiliary = overlay_win;

	return 0;
}