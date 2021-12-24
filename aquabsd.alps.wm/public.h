#if !defined(__AQUABSD_ALPS_WM)
#define __AQUABSD_ALPS_WM

#include <aquabsd.alps.win/public.h>

#include <xcb/composite.h>

typedef enum {
	AQUABSD_ALPS_WM_CB_CREATE,
	AQUABSD_ALPS_WM_CB_SHOW,
	AQUABSD_ALPS_WM_CB_HIDE,
	AQUABSD_ALPS_WM_CB_MODIFY,
	AQUABSD_ALPS_WM_CB_DELETE,
	AQUABSD_ALPS_WM_CB_FOCUS,
	AQUABSD_ALPS_WM_CB_LEN
} aquabsd_alps_wm_cb_t;

typedef struct {
	aquabsd_alps_win_t* root;
	
	// X11 atoms (used for communicating information between clients)

	xcb_window_t support_win;

	xcb_atom_t client_list_atom;
	xcb_atom_t supported_atoms_list_atom;

	// doubly-linked list with all windows

	unsigned win_count;

	aquabsd_alps_win_t* win_head;
	aquabsd_alps_win_t* win_tail;

	// app client callbacks
	// a bit of data-orientated design here ;)

	uint64_t cbs[AQUABSD_ALPS_WM_CB_LEN];
	uint64_t cb_params[AQUABSD_ALPS_WM_CB_LEN];
} aquabsd_alps_wm_t;

// functions exposed to devices and apps

static aquabsd_alps_wm_t* (*aquabsd_alps_wm_create) (void);
static void (*aquabsd_alps_wm_delete) (aquabsd_alps_wm_t* wm);

static aquabsd_alps_win_t* (*aquabsd_alps_wm_get_root_win) (aquabsd_alps_wm_t* wm);

static unsigned (*aquabsd_alps_wm_get_x_res) (aquabsd_alps_wm_t* wm);
static unsigned (*aquabsd_alps_wm_get_y_res) (aquabsd_alps_wm_t* wm);

static int (*aquabsd_alps_wm_set_name) (aquabsd_alps_wm_t* wm, const char* name);

static int (*aquabsd_alps_wm_register_cb) (aquabsd_alps_wm_t* wm, aquabsd_alps_wm_cb_t type, uint64_t cb, uint64_t param);
static int (*aquabsd_alps_wm_make_compositing) (aquabsd_alps_wm_t* wm);

#endif
