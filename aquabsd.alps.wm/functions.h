#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xcb/randr.h>

#define SUPER_MOD XCB_MOD_MASK_4 // looks like this is the super key

#define LOG_SIGNATURE "[aquabsd.alps.wm]"

#define LOG_REGULAR "\033[0m"
#define LOG_RED     "\033[0;31m"
#define LOG_GREEN   "\033[0;32m"
#define LOG_YELLOW  "\033[0;33m"

#define FATAL_ERROR(...) \
	fprintf(stderr, LOG_SIGNATURE LOG_RED " FATAL ERROR "__VA_ARGS__); \
	fprintf(stderr, LOG_REGULAR); \
	\
	delete(wm); \
	return NULL;

#define WARN(...) \
	fprintf(stderr, LOG_SIGNATURE LOG_YELLOW " WARNING "__VA_ARGS__); \
	fprintf(stderr, LOG_REGULAR);

// helper functions (for XCB)

static inline xcb_atom_t get_intern_atom(wm_t* wm, const char* name) {
	// TODO obviously, this function isn't super ideal for leveraging the benefits XCB provides over Xlib
	//      at some point, refactor this so that... well all this work converting from Xlib to XCB isn't for nothing

	xcb_intern_atom_cookie_t atom_cookie = xcb_intern_atom(wm->root->connection, 0, strlen(name), name);
	xcb_intern_atom_reply_t* atom_reply = xcb_intern_atom_reply(wm->root->connection, atom_cookie, NULL);
	
	xcb_atom_t atom = atom_reply->atom;
	free(atom_reply);

	return atom;
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
	xcb_window_t* client_list = calloc(wm->win_count, sizeof *client_list);
	unsigned i = 0;

	for (win_t* win = wm->win_head; win; win = win->next) {
		client_list[i++] = win->win;
	}

	xcb_change_property(wm->root->connection, XCB_PROP_MODE_REPLACE, wm->root->win, wm->root->ewmh._NET_CLIENT_LIST, XCB_ATOM_WINDOW, 32, wm->win_count, client_list);
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
		fprintf(stderr, "[aquabsd.alps.wm] WARN Window of id 0x%x was not found\n", id);
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

	float x = (float) detail->root_x / wm->root->x_res;
	float y = 1.0 - (float) detail->root_y / wm->root->y_res;

	// TODO 4 arguments is too much for a KOS callback (can probably remove the wm parameter; it's a bit redundant)
	// return kos_callback(cb, 4, (uint64_t) wm, *(uint64_t*) &x, *(uint64_t*) &y, param);

	return kos_callback(cb, 3, *(uint64_t*) &x, *(uint64_t*) &y, param);
}

static void _state_update(wm_t* wm, win_t* win) {
	uint32_t len = 0;
	xcb_atom_t atoms[16 /* provision */];

	if (win->state == AQUABSD_ALPS_WIN_STATE_FULLSCREEN) {
		atoms[len++] = wm->root->ewmh._NET_WM_STATE_FULLSCREEN;
	}

	xcb_ewmh_set_wm_state(&wm->root->ewmh, win->win, len, atoms);
}

static inline void __state_regular(wm_t* wm, win_t* win) {
	if (win->state == AQUABSD_ALPS_WIN_STATE_REGULAR) {
		return;
	}

	win->state = AQUABSD_ALPS_WIN_STATE_REGULAR;
	_state_update(wm, win);
}

static inline void __state_fullscreen(wm_t* wm, win_t* win) {
	if (win->state == AQUABSD_ALPS_WIN_STATE_FULLSCREEN) {
		return;
	}

	win->state = AQUABSD_ALPS_WIN_STATE_FULLSCREEN;
	_state_update(wm, win);
}

static int process_state(wm_t* wm, win_t* win, xcb_client_message_event_t* detail) {
	if (!win) {
		return -1;
	}

	uint32_t action = detail->data.data32[0];
	uint32_t type = detail->data.data32[1];

	if (type == wm->root->ewmh._NET_WM_STATE_FULLSCREEN) {
		if (action == XCB_EWMH_WM_STATE_ADD) {
			__state_fullscreen(wm, win);
		}

		else if (action == XCB_EWMH_WM_STATE_REMOVE) {
			__state_regular(wm, win);
		}

		else if (action == XCB_EWMH_WM_STATE_TOGGLE) {
			if (win->state == AQUABSD_ALPS_WIN_STATE_FULLSCREEN) {
				__state_regular(wm, win);
			}

			else {
				__state_fullscreen(wm, win);
			}
		}

		else {
			return -1;
		}
	}

	else {
		return -1; // not a type we know of
	}

	return call_cb(wm, win, CB_STATE);
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

		const uint32_t attribs[] = {
			XCB_EVENT_MASK_FOCUS_CHANGE
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

		WIN_CONFIG
		modify_win(wm, win);
	}

	else if (type == XCB_FOCUS_IN) { // focus window
		xcb_focus_in_event_t* detail = (void*) event;
		focus_win(wm, search_win(wm, detail->event));
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

		wm->in_wm_click = click_intended_for_us(wm, detail);

		if (wm->in_wm_click) {
			// prevent event from passing through to the client

			xcb_grab_pointer(wm->root->connection, 1, wm->root->win, XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_CURRENT_TIME);
			xcb_grab_button(wm->root->connection, 1, wm->root->win, XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_BUTTON_INDEX_ANY, XCB_BUTTON_MASK_ANY);
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
			xcb_grab_button(wm->root->connection, 1, wm->root->win, XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_BUTTON_INDEX_ANY, SUPER_MOD);
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

		if (detail->type == wm->root->ewmh._NET_WM_STATE) {
			process_state(wm, win, detail);
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

	// xcb_grab_button(wm->root->connection, 1, wm->root->win, XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_BUTTON_INDEX_ANY, XCB_BUTTON_MASK_ANY);
	// xcb_ungrab_button(wm->root->connection, XCB_BUTTON_INDEX_ANY, wm->root->win, XCB_GRAB_ANY);
	xcb_grab_button(wm->root->connection, 1, wm->root->win, XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_MOTION, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, XCB_NONE, XCB_NONE, XCB_BUTTON_INDEX_ANY, SUPER_MOD);

	// grab the keys we are interested in as a window manager (i.e. those with the super key modifier)

	xcb_ungrab_key(wm->root->connection, XCB_GRAB_ANY, wm->root->win, XCB_MOD_MASK_ANY);
	xcb_grab_key(wm->root->connection, 1, wm->root->win, SUPER_MOD, XCB_GRAB_ANY, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);

	// now, we move on to '_NET_SUPPORTING_WM_CHECK'
	// this is a bit weird, but it's all specified by the EWMH spec: https://developer.gnome.org/wm-spec/

	wm->support_win = create_dumb_win(wm);
	xcb_window_t support_win_list[1] = { wm->support_win };

	xcb_change_property(wm->root->connection, XCB_PROP_MODE_REPLACE, wm->root->win, wm->root->ewmh._NET_SUPPORTING_WM_CHECK, XCB_ATOM_WINDOW, 32, 1, support_win_list);
	xcb_change_property(wm->root->connection, XCB_PROP_MODE_REPLACE, wm->support_win, wm->root->ewmh._NET_SUPPORTING_WM_CHECK, XCB_ATOM_WINDOW, 32, 1, support_win_list);

	// specify which atoms are supported in '_NET_SUPPORTED'

	const xcb_atom_t supported_atoms[] = {
		wm->root->ewmh._NET_SUPPORTED,
		wm->root->ewmh._NET_SUPPORTING_WM_CHECK,
		wm->root->ewmh._NET_CLIENT_LIST,

		wm->root->ewmh._NET_WM_VISIBLE_NAME,
		wm->root->ewmh._NET_WM_NAME,

		wm->root->ewmh._NET_WM_STATE,
		wm->root->ewmh._NET_WM_STATE_FULLSCREEN,
	};

	xcb_change_property(wm->root->connection, XCB_PROP_MODE_REPLACE, wm->root->win, wm->root->ewmh._NET_SUPPORTED, XCB_ATOM_ATOM, 32, sizeof(supported_atoms) / sizeof(*supported_atoms), supported_atoms);

	// get all monitors and their geometries
	// CRTC stands for "Cathode Ray Tube Controller", which is a dumb name, so AQUA calls them "providers"

	xcb_randr_get_screen_resources_cookie_t res_cookie = xcb_randr_get_screen_resources(wm->root->connection, wm->root->win);
	xcb_randr_get_screen_resources_reply_t* res = xcb_randr_get_screen_resources_reply(wm->root->connection, res_cookie, NULL);

	if (!res) {
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
			goto skip;
		}

		wm->providers = realloc(wm->providers, ++wm->provider_count * sizeof *wm->providers);
		provider_t* provider = &wm->providers[wm->provider_count - 1];

		provider->x = info->x;
		provider->y = info->y;

		provider->x_res = info->width;
		provider->y_res = info->height;

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

	if (wm->root->default_screen > 99) {
		return -1;
	}

	xcb_atom_t atom = wm->root->ewmh._NET_WM_CM_Sn[wm->root->default_screen];
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
	xcb_composite_get_overlay_window_reply_t* overlay_win_reply = xcb_composite_get_overlay_window_reply(wm->root->connection, overlay_win_cookie, NULL);

	xcb_window_t overlay_win = overlay_win_reply->overlay_win;
	free(overlay_win_reply);

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