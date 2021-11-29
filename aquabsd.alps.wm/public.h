#if !defined(__AQUABSD_ALPS_WM)
#define __AQUABSD_ALPS_WM

#include <aquabsd.alps.win/public.h>

typedef struct {
	char* name;
	unsigned x_res, y_res;

	// X11 generics

	Display* display;
	int default_screen;

	xcb_connection_t* connection;
	xcb_screen_t* screen;

	xcb_window_t root;

	// X11 atoms

	xcb_atom_t client_list_atom;
} aquabsd_alps_wm_t;

#endif
