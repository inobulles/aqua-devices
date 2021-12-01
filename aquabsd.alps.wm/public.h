#if !defined(__AQUABSD_ALPS_WM)
#define __AQUABSD_ALPS_WM

#include <aquabsd.alps.win/public.h>

#include <xcb/composite.h>

typedef enum {
	AQUABSD_ALPS_WM_CB_CREATE,
	AQUABSD_ALPS_WM_CB_MODIFY,
	AQUABSD_ALPS_WM_CB_DELETE,
	AQUABSD_ALPS_WM_CB_LEN
} aquabsd_alps_wm_cb_t;

typedef struct aquabsd_alps_wm_win_t aquabsd_alps_wm_win_t; // forward declaration

struct aquabsd_alps_wm_win_t { // this is all the WM really cares about as for what a window is
	xcb_window_t win;

	unsigned visible;

	int x, y;
	unsigned x_res, y_res;

	// for doubly-linked list of windows

	aquabsd_alps_wm_win_t* prev;
	aquabsd_alps_wm_win_t* next;
};

typedef struct {
	aquabsd_alps_win_t* root;
	unsigned compositing; // explained in more detail in 'make_compositing'
	
	// X11 atoms (used for communicating information between clients)

	xcb_atom_t client_list_atom;

	// doubly-linked list with all windows

	unsigned win_count;

	aquabsd_alps_wm_win_t* win_head;
	aquabsd_alps_wm_win_t* win_tail;

	// app client callbacks
	// a bit of data-orientated design here ;)

	uint64_t cbs[AQUABSD_ALPS_WM_CB_LEN];
	uint64_t cb_params[AQUABSD_ALPS_WM_CB_LEN];
} aquabsd_alps_wm_t;

#endif
