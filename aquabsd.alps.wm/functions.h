#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FATAL_ERROR(...) fprintf(stderr, "[aquabsd.alps.wm] FATAL ERROR "__VA_ARGS__); delete(wm); return NULL;
#define WARNING(...) fprintf(stderr, "[aquabsd.alps.wm] WARNING "__VA_ARGS__);

static xcb_atom_t get_intern_atom(wm_t* wm, const char* name) {
	// TODO obviously, this function isn't super ideal for leveraging the benefits XCB provides over Xlib
	//      at some point, refactor this so that... well all this work converting from Xlib to XCB isn't for nothing

	xcb_intern_atom_cookie_t atom_cookie = xcb_intern_atom(wm->connection, 1, strlen(name), name);
	return xcb_intern_atom_reply(wm->connection, atom_cookie, NULL)->atom;
}

dynamic int delete(wm_t* wm) {
	if (wm->name) free(wm->name);

	if (wm->display) XCloseDisplay(wm->display);

	free(wm);

	return 0;
}

dynamic wm_t* create(const char* name) {
	wm_t* wm = calloc(1, sizeof *wm);
	wm->name = strdup(name);

	wm->display = XOpenDisplay(NULL /* default to 'DISPLAY' environment variable */);

	if (!wm->display) {
		FATAL_ERROR("Failed to open X display\n")
	}

	// setup XCB connection

	wm->default_screen = DefaultScreen(wm->display);
	wm->connection = XGetXCBConnection(wm->display);

	if (!wm->connection) {
		FATAL_ERROR("Failed to get XCB connection from X display\n")
	}

	XSetEventQueueOwner(wm->display, XCBOwnsEventQueue);

	// get screen and root window

	xcb_screen_iterator_t it = xcb_setup_roots_iterator(xcb_get_setup(wm->connection));
	for (int i = wm->default_screen; it.rem && i > 0; i--, xcb_screen_next(&it));
	
	wm->screen = it.data;
	wm->root = wm->screen->root;

	// get width & height of root window

	xcb_get_geometry_cookie_t root_geom_cookie = xcb_get_geometry(wm->connection, wm->root);
	xcb_get_geometry_reply_t* root_geom = xcb_get_geometry_reply(wm->connection, root_geom_cookie, NULL);

	wm->x_res = root_geom->width;
	wm->y_res = root_geom->height;

	// TODO make this comment correct
	// tell X to send us all 'CreateNotify', 'ConfigureNotify', and 'DestroyNotify' events ('SubstructureNotifyMask' also sends back some other events but we're not using those)

	const uint32_t attribs[] = {
		XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | XCB_EVENT_MASK_PROPERTY_CHANGE
	};

	xcb_change_window_attributes(wm->connection, wm->root, XCB_CW_EVENT_MASK, attribs);

	// grab the keys we are interested in as a window manager (i.e. those with the super key modifier)

	xcb_ungrab_key(wm->connection, XCB_GRAB_ANY, wm->root, XCB_MOD_MASK_ANY);
	xcb_grab_key(wm->connection, 1, wm->root, XCB_MOD_MASK_4 /* looks like this is the super key */, XCB_GRAB_ANY, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

	// TODO actually explain shit in 'wm_t'
	// setup our atoms (explained in more detail in the 'wm_t' struct)
	// we also need to specify which atoms are supported in '_NET_SUPPORTED'

	wm->client_list_atom = get_intern_atom(wm, "_NET_CLIENT_LIST");

	xcb_atom_t supported_atoms_list_atom = get_intern_atom(wm, "_NET_SUPPORTED");
	const xcb_atom_t supported_atoms[] = { supported_atoms_list_atom, wm->client_list_atom };

	xcb_change_property(wm->connection, XCB_PROP_MODE_REPLACE, wm->root, supported_atoms_list_atom, XA_ATOM, 32, sizeof(supported_atoms) / sizeof(*supported_atoms), supported_atoms);

	// now, we move on to '_NET_SUPPORTING_WM_CHECK'
	// this is a bit weird, but it's all specified by the EWMH spec: https://developer.gnome.org/wm-spec/

	xcb_atom_t supporting_wm_check_atom = get_intern_atom(wm, "_NET_SUPPORTING_WM_CHECK");

	xcb_window_t support_win = xcb_generate_id(wm->connection);
	xcb_create_window(wm->connection, XCB_COPY_FROM_PARENT, support_win, wm->root, 0, 0, 1, 1, 0, XCB_WINDOW_CLASS_COPY_FROM_PARENT, 0, 0, NULL);

	xcb_window_t support_win_list[1] = { support_win };

	xcb_change_property(wm->connection, XCB_PROP_MODE_REPLACE, wm->root, supporting_wm_check_atom, XA_WINDOW, 32, 1, support_win_list);
	xcb_change_property(wm->connection, XCB_PROP_MODE_REPLACE, support_win, supporting_wm_check_atom, XA_WINDOW, 32, 1, support_win_list);

	xcb_atom_t name_atom = get_intern_atom(wm, "_NET_WM_NAME");
	xcb_change_property(wm->connection, XCB_PROP_MODE_REPLACE, support_win, name_atom, XCB_ATOM_STRING, 8, strlen(wm->name) + 1, wm->name);

	// TODO get all monitors and their individual resolutions with Xinerama

	// flush

	xcb_flush(wm->connection);

	return wm;
}