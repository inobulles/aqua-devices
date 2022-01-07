#if !defined(__AQUABSD_ALPS_WIN)
#define __AQUABSD_ALPS_WIN

#include <aquabsd.alps.mouse/public.h>
#include <aquabsd.alps.kbd/public.h>

#include <pthread.h>

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_event.h>

// for compatibility with EGL/GLX, we must use Xlib alongside XCB:
// https://xcb.freedesktop.org/opengl/

#include <X11/Xlib.h>
#include <X11/Xutil.h> // for 'XLookupString'
#include <X11/Xlib-xcb.h>

typedef enum {
	AQUABSD_ALPS_WIN_CB_DRAW,
	AQUABSD_ALPS_WIN_CB_LEN
} aquabsd_alps_win_cb_t;

typedef struct aquabsd_alps_win_t aquabsd_alps_win_t; // forward declaration

struct aquabsd_alps_win_t {
	unsigned running;
	unsigned visible;
	
	unsigned x_pos, y_pos;
	unsigned x_res, y_res;

	// information about the window manager

	unsigned wm_x_res, wm_y_res;

	// X11 stuff

	Display* display;
	int default_screen;

	xcb_connection_t* connection;
	xcb_screen_t* screen;

	xcb_window_t win;
	xcb_window_t auxiliary; // in case we wanted to process events from one window but draw to another

	// atoms

	xcb_atom_t wm_protocols_atom;
	xcb_atom_t wm_delete_win_atom;

	xcb_atom_t _net_wm_visible_name_atom;
	xcb_atom_t _net_wm_name_atom;

	// multithreading stuff

	unsigned threading_enabled;
	pthread_t event_thread;

	// app client callbacks
	// a bit of data-orientated design here ;)

	uint64_t cbs[AQUABSD_ALPS_WIN_CB_LEN];
	uint64_t cb_params[AQUABSD_ALPS_WIN_CB_LEN];

	// device client callbacks

	int (*dev_cbs[AQUABSD_ALPS_WIN_CB_LEN]) (aquabsd_alps_win_t* win, void* param, uint64_t cb, uint64_t cb_param);
	void* dev_cb_params[AQUABSD_ALPS_WIN_CB_LEN];

	// mouse input stuff

	unsigned mouse_buttons[AQUABSD_ALPS_MOUSE_BUTTON_COUNT];
	float mouse_axes[AQUABSD_ALPS_MOUSE_AXIS_COUNT];

	int mouse_x, mouse_y;

	// keyboard input stuff

	unsigned kbd_buttons[AQUABSD_ALPS_KBD_BUTTON_COUNT];

	unsigned kbd_buf_len;
	void* kbd_buf;

	// in case we need a doubly-linked list of windows

	aquabsd_alps_win_t* prev;
	aquabsd_alps_win_t* next;

	// potential EGL stuff, if needed
	// these fields are used by the aquabsd.alps.ogl device and should not be accessed by us

	unsigned pixmap_modified;

	xcb_pixmap_t egl_pixmap;
	uint32_t /* GLuint */ egl_texture;
	void* /* EGLImageKHR */ egl_image;
	// void* /* EGLSurface */ egl_surface;

	// potential WM stuff, if needed
	// these fields are used by the aquabsd.alps.wm device, and we should use them for what they are intended

	xcb_button_t wm_prev_button; // cf. process_events
	void* wm_object;
	int (*wm_event_cb) (void* _wm, int type, xcb_generic_event_t* event);
};

// functions exposed to devices & apps

static aquabsd_alps_win_t* (*aquabsd_alps_win_create) (unsigned x_res, unsigned y_res);
static int (*aquabsd_alps_win_delete) (aquabsd_alps_win_t* win);

static int (*aquabsd_alps_win_set_caption) (aquabsd_alps_win_t* win, const char* caption);
static char* (*aquabsd_alps_win_get_caption) (aquabsd_alps_win_t* win);

static int (*aquabsd_alps_win_register_cb) (aquabsd_alps_win_t* win, aquabsd_alps_win_cb_t type, uint64_t cb, uint64_t param);
static int (*aquabsd_alps_win_loop) (aquabsd_alps_win_t* win);

static int (*aquabsd_alps_win_close_win) (aquabsd_alps_win_t* win);
static int (*aquabsd_alps_win_grab_focus) (aquabsd_alps_win_t* win);

static int (*aquabsd_alps_win_move) (aquabsd_alps_win_t* win, float x, float y);
static int (*aquabsd_alps_win_resize) (aquabsd_alps_win_t* win, float x, float y);

static int (*aquabsd_alps_win_get_x_pos) (aquabsd_alps_win_t* win);
static int (*aquabsd_alps_win_get_y_pos) (aquabsd_alps_win_t* win);

static unsigned (*aquabsd_alps_win_get_x_res) (aquabsd_alps_win_t* win);
static unsigned (*aquabsd_alps_win_get_y_res) (aquabsd_alps_win_t* win);

static unsigned (*aquabsd_alps_win_get_wm_x_res) (aquabsd_alps_win_t* win);
static unsigned (*aquabsd_alps_win_get_wm_y_res) (aquabsd_alps_win_t* win);

// functions exposed exclusively to devices

static aquabsd_alps_win_t* (*aquabsd_alps_win_create_setup) (void);
static void (*aquabsd_alps_win_get_ewmh_atoms) (aquabsd_alps_win_t* win);

static int (*aquabsd_alps_win_register_dev_cb) (aquabsd_alps_win_t* win, aquabsd_alps_win_cb_t type, int (*cb) (aquabsd_alps_win_t* win, void* param, uint64_t cb, uint64_t cb_param), void* param);

static xcb_window_t (*aquabsd_alps_win_get_draw_win) (aquabsd_alps_win_t* win);

#endif
