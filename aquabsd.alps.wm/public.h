#if !defined(__AQUABSD_ALPS_WM)
#define __AQUABSD_ALPS_WM

#include <aquabsd.alps.win/public.h>

#include <xcb/composite.h>

typedef enum {
	AQUABSD_ALPS_WM_CB_CREATE, // new window has been created
	AQUABSD_ALPS_WM_CB_SHOW,   // window has been shown (mapped in X11 terminology)
	AQUABSD_ALPS_WM_CB_HIDE,   // window has been hidden (unmapped in X11 terminology)
	AQUABSD_ALPS_WM_CB_MODIFY, // window has been modified (i.e. resized or moved or both)
	AQUABSD_ALPS_WM_CB_DELETE, // window has been closed
	AQUABSD_ALPS_WM_CB_FOCUS,  // window has been focused
	AQUABSD_ALPS_WM_CB_STATE,  // window's state has been changed
	AQUABSD_ALPS_WM_CB_CLICK,  // determine whether a click event is intended for the WM or should be passed on to the window underneath the cursor
	AQUABSD_ALPS_WM_CB_LEN
} aquabsd_alps_wm_cb_t;

typedef struct {
	int x, y;
	unsigned x_res, y_res;
} aquabsd_alps_wm_provider_t;

typedef struct {
	aquabsd_alps_win_t* root;
	
	// X11 atoms (used for communicating information between clients)

	xcb_window_t support_win;

	// event stuff

	unsigned in_wm_click; // is the WM currently processing a click intended for itself?

	// doubly-linked list with all windows

	unsigned win_count;

	aquabsd_alps_win_t* win_head;
	aquabsd_alps_win_t* win_tail;

	// geometry of all monitors ("providers")

	unsigned provider_count;
	aquabsd_alps_wm_provider_t* providers;

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

// provider stuff

static int (*aquabsd_alps_wm_provider_count) (aquabsd_alps_wm_t* wm);

static int (*aquabsd_alps_wm_provider_x) (aquabsd_alps_wm_t* wm, unsigned id);
static int (*aquabsd_alps_wm_provider_y) (aquabsd_alps_wm_t* wm, unsigned id);

static int (*aquabsd_alps_wm_provider_x_res) (aquabsd_alps_wm_t* wm, unsigned id);
static int (*aquabsd_alps_wm_provider_y_res) (aquabsd_alps_wm_t* wm, unsigned id);

#endif
