#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FATAL_ERROR(...) fprintf(stderr, "[aquabsd.alps.wm] FATAL ERROR "__VA_ARGS__); delete(wm); return NULL;
#define WARNING(...) fprintf(stderr, "[aquabsd.alps.wm] WARNING "__VA_ARGS__);

static xcb_atom_t get_intern_atom(wm_t* wm, const char* name) {
	// TODO obviously, this function isn't super ideal for leveraging the benefits XCB provides over Xlib
	//      at some point, refactor this so that... well all this work converting from Xlib to XCB isn't for nothing

	xcb_intern_atom_cookie_t atom_cookie = xcb_intern_atom(wm->root->connection, 0, strlen(name), name);
	return xcb_intern_atom_reply(wm->root->connection, atom_cookie, NULL)->atom;
}

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
	win->visible = 1;
	// TODO see how I want to handle callbacks for this one
	printf("show_win\n");
}

static void hide_win(wm_t* wm, win_t* win) {
	win->visible = 0;
	// TODO see how I want to handle callbacks for this one
	printf("hide_win\n");
}

static void modify_win(wm_t* wm, win_t* win) {
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
	win->x = detail->x; \
	win->y = detail->y; \
	\
	win->x_res = detail->width; \
	win->y_res = detail->height;

static int process_event(void* _wm, int type, xcb_generic_event_t* event) {
	wm_t* wm = _wm;
	
	// all of the definitions of these structs can be found in <xcb/xproto.h>

	if (type == XCB_CREATE_NOTIFY) {
		xcb_create_notify_event_t* detail = (void*) event;
		
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

	else {
		printf("unknown %d\n", type);
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
		XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
	};

	xcb_change_window_attributes(wm->root->connection, wm->root->win, XCB_CW_EVENT_MASK, attribs);

	// grab the keys we are interested in as a window manager (i.e. those with the super key modifier)

	xcb_ungrab_key(wm->root->connection, XCB_GRAB_ANY, wm->root->win, XCB_MOD_MASK_ANY);
	xcb_grab_key(wm->root->connection, 1, wm->root->win, XCB_MOD_MASK_4 /* looks like this is the super key */, XCB_GRAB_ANY, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

	// setup our atoms
	// we also need to specify which atoms are supported in '_NET_SUPPORTED'

	wm->client_list_atom = get_intern_atom(wm, "_NET_CLIENT_LIST");

	xcb_atom_t supported_atoms_list_atom = get_intern_atom(wm, "_NET_SUPPORTED");
	const xcb_atom_t supported_atoms[] = { supported_atoms_list_atom, wm->client_list_atom };

	xcb_change_property(wm->root->connection, XCB_PROP_MODE_REPLACE, wm->root->win, supported_atoms_list_atom, XCB_ATOM_ATOM, 32, sizeof(supported_atoms) / sizeof(*supported_atoms), supported_atoms);

	// now, we move on to '_NET_SUPPORTING_WM_CHECK'
	// this is a bit weird, but it's all specified by the EWMH spec: https://developer.gnome.org/wm-spec/

	xcb_atom_t supporting_wm_check_atom = get_intern_atom(wm, "_NET_SUPPORTING_WM_CHECK");

	xcb_window_t support_win = xcb_generate_id(wm->root->connection);
	xcb_create_window(wm->root->connection, XCB_COPY_FROM_PARENT, support_win, wm->root->win, 0, 0, 1, 1, 0, XCB_WINDOW_CLASS_COPY_FROM_PARENT, 0, 0, NULL);

	xcb_window_t support_win_list[1] = { support_win };

	xcb_change_property(wm->root->connection, XCB_PROP_MODE_REPLACE, wm->root->win, supporting_wm_check_atom, XCB_ATOM_WINDOW, 32, 1, support_win_list);
	xcb_change_property(wm->root->connection, XCB_PROP_MODE_REPLACE, support_win, supporting_wm_check_atom, XCB_ATOM_WINDOW, 32, 1, support_win_list);

	// TODO get all monitors and their individual resolutions with Xinerama

	// flush

	xcb_flush(wm->root->connection);

	// set the fields reserved to us in the root window object

	wm->root->wm_object = wm;
	wm->root->wm_event_cb = process_event;

	return wm;
}

dynamic aquabsd_alps_win_t* get_root_win(wm_t* wm) {
	return wm->root;
}

dynamic unsigned get_x_res(wm_t* wm) {
	return wm->root->x_res;
}

dynamic unsigned get_y_res(wm_t* wm) {
	return wm->root->y_res;
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

