#if !defined(__AQUABSD_ALPS_WIN)
#define __AQUABSD_ALPS_WIN

#include <aquabsd.alps.mouse/public.h>

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_event.h>

// for compatibility with EGL/GLX, we must use Xlib alongside XCB:
// https://xcb.freedesktop.org/opengl/

#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>

typedef enum {
	AQUABSD_ALPS_WIN_CB_DRAW,
	AQUABSD_ALPS_WIN_CB_LEN
} aquabsd_alps_win_cb_t;

typedef struct aquabsd_alps_win_t aquabsd_alps_win_t; // forward declaration

struct aquabsd_alps_win_t { 
	// X11 stuff

	unsigned x_res, y_res;

	Display* display;
	int default_screen;

	xcb_connection_t* connection;
	xcb_screen_t* screen;

	xcb_drawable_t win;

	xcb_atom_t wm_delete_win_atom;

	// app client callbacks
	// a bit of data-orientated design here ;)

	uint64_t cbs[AQUABSD_ALPS_WIN_CB_LEN];
	uint64_t cb_params[AQUABSD_ALPS_WIN_CB_LEN];

	// device client callbacks

	int (*dev_cbs[AQUABSD_ALPS_WIN_CB_LEN]) (aquabsd_alps_win_t* win, void* param, uint64_t cb, uint64_t cb_param);
	void* dev_cb_params[AQUABSD_ALPS_WIN_CB_LEN];

	// input tracking stuff

	unsigned mouse_buttons[AQUABSD_ALPS_MOUSE_BUTTON_COUNT];
	float mouse_axes[AQUABSD_ALPS_MOUSE_AXIS_COUNT];

	int mouse_x, mouse_y;
};

// functions exposed to devices & apps

static aquabsd_alps_win_t* (*aquabsd_alps_win_create) (unsigned x_res, unsigned y_res);
static int (*aquabsd_alps_win_delete) (aquabsd_alps_win_t* win);

static int (*aquabsd_alps_win_register_cb) (aquabsd_alps_win_t* win, aquabsd_alps_win_cb_t type, uint64_t cb, uint64_t param);
static int (*aquabsd_alps_win_loop) (aquabsd_alps_win_t* win);

// functions exposed exclusively to devices

static int (*aquabsd_alps_win_register_dev_cb) (aquabsd_alps_win_t* win, aquabsd_alps_win_cb_t type, int (*cb) (aquabsd_alps_win_t* win, void* param, uint64_t cb, uint64_t cb_param), void* param);

#endif