#if !defined(__AQUABSD_ALPS_WM)
#define __AQUABSD_ALPS_WM

#include <aquabsd.alps.win/public.h>

typedef struct {
	char* name;
	aquabsd_alps_win_t* root;
	
	// X11 atoms

	xcb_atom_t client_list_atom;
} aquabsd_alps_wm_t;

#endif
