#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	if (wm->root) aquabsd_alps_win_delete(wm->root);

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
	// in a compositing window manager, this means we're trying to show the overlay
	// prevent that

	if (win->win == wm->root->auxiliary) {
		return;
	}

	win->visible = 1;
	call_cb(wm, win, CB_SHOW);
}

static void hide_win(wm_t* wm, win_t* win) {
	win->visible = 0;
	call_cb(wm, win, CB_HIDE);
}

static void modify_win(wm_t* wm, win_t* win) {
	win->pixmap_modified = 1;
	call_cb(wm, win, CB_MODIFY);
}

static void rem_win(wm_t* wm, win_t* win) {
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

#define WIN_CONFIG \
	win->x_pos = detail->x; \
	win->y_pos = detail->y; \
	\
	win->x_res = detail->width; \
	win->y_res = detail->height;

static int process_event(void* _wm, int type, xcb_generic_event_t* event) {
	wm_t* wm = _wm;
	
	// window management events
	// all of the definitions of these structs can be found in <xcb/xproto.h>

	if (type == XCB_CREATE_NOTIFY) {
		xcb_create_notify_event_t* detail = (void*) event;

		const uint32_t attribs[] = {
			XCB_EVENT_MASK_FOCUS_CHANGE
		};

		// make it so that we receive this window's mouse events too

		xcb_change_window_attributes(wm->root->connection, detail->window, XCB_CW_EVENT_MASK, attribs);
		xcb_grab_button(wm->root->connection, 1, detail->window, XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION, XCB_GRAB_MODE_SYNC, XCB_GRAB_MODE_SYNC, XCB_NONE, XCB_NONE, XCB_BUTTON_INDEX_ANY, XCB_MOD_MASK_ANY);

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

	else if (type == XCB_BUTTON_PRESS) {
		xcb_button_press_event_t* detail = (void*) event;

		xcb_allow_events(wm->root->connection, XCB_ALLOW_REPLAY_POINTER, XCB_CURRENT_TIME); // pass on event to client
		// xcb_allow_events(wm->root->connection, XCB_ALLOW_SYNC_POINTER, XCB_CURRENT_TIME); // don't pass on event to client
	}

	else if (type == XCB_BUTTON_RELEASE) {
		xcb_button_release_event_t* detail = (void*) event;
		xcb_allow_events(wm->root->connection, XCB_ALLOW_REPLAY_POINTER, XCB_CURRENT_TIME);
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

	xcb_get_geometry_cookie_t root_geom_cookie = xcb_get_geometry(wm->root->connection, wm->root->win);
	xcb_get_geometry_reply_t* root_geom = xcb_get_geometry_reply(wm->root->connection, root_geom_cookie, NULL);

	wm->root->x_res = root_geom->width;
	wm->root->y_res = root_geom->height;

	// TODO make this comment correct
	// tell X to send us all 'CreateNotify', 'ConfigureNotify', and 'DestroyNotify' events ('SubstructureNotifyMask' also sends back some other events but we're not using those)

	const uint32_t attribs[] = {
		//XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_PROPERTY_CHANGE
		XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY |
		XCB_EVENT_MASK_EXPOSURE |
		XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW | XCB_EVENT_MASK_POINTER_MOTION |
		XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE
	};

	xcb_change_window_attributes(wm->root->connection, wm->root->win, XCB_CW_EVENT_MASK, attribs);

	// grab the keys we are interested in as a window manager (i.e. those with the super key modifier)

	xcb_ungrab_key(wm->root->connection, XCB_GRAB_ANY, wm->root->win, XCB_MOD_MASK_ANY);
	xcb_grab_key(wm->root->connection, 1, wm->root->win, XCB_MOD_MASK_4 /* looks like this is the super key */, XCB_GRAB_ANY, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

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

	// TODO get all monitors and their individual resolutions with Xinerama

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