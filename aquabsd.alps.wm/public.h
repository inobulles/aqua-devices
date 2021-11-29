#if !defined(__AQUABSD_ALPS_WM)
#define __AQUABSD_ALPS_WM

#include <aquabsd.alps.win/public.h>

typedef enum {
	AQUABSD_ALPS_WM_CB_CREATE,
	AQUABSD_ALPS_WM_CB_MODIFY,
	AQUABSD_ALPS_WM_CB_DELETE,
	AQUABSD_ALPS_WM_CB_LEN
} aquabsd_alps_wm_cb_t;

typedef struct {
	aquabsd_alps_win_t* root;
	
	// X11 atoms (used for communicating information between clients)

	xcb_atom_t client_list_atom;

	// app client callbacks
	// a bit of data-orientated design here ;)

	uint64_t cbs[AQUABSD_ALPS_WM_CB_LEN];
	uint64_t cb_params[AQUABSD_ALPS_WM_CB_LEN];
} aquabsd_alps_wm_t;

#endif
