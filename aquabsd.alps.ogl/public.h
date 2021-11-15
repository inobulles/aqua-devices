#if !defined(__AQUABSD_ALPS_OGL)
#define __AQUABSD_ALPS_OGL

#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_event.h>

// unfortunately, you can't use pure XCB with EGL/GLX:
// https://xcb.freedesktop.org/opengl/

#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>

#include <GL/gl.h>
#include <EGL/egl.h>

typedef enum {
	AQUABSD_ALPS_OGL_CB_DRAW,
	AQUABSD_ALPS_OGL_CB_LEN
} aquabsd_alps_ogl_cb_t;

typedef struct {
} aquabsd_alps_ogl_window_t;

typedef struct {
	union {
		aquabsd_alps_ogl_window_t* window;
		// aquabsd_alps_ogl_wm_t* wm;
	} backend;

	unsigned x_res, y_res;

	// X11 stuff

	Display* display;
	int default_screen;

	xcb_connection_t* connection;
	xcb_screen_t* screen;

	xcb_drawable_t window;

	xcb_atom_t wm_delete_window_atom;

	// EGL stuff

	EGLDisplay* egl_display;
	EGLContext* egl_context;
	EGLSurface* egl_surface;

	// callbacks

	uint64_t cbs[AQUABSD_ALPS_OGL_CB_LEN];
	uint64_t cb_params[AQUABSD_ALPS_OGL_CB_LEN];
} aquabsd_alps_ogl_context_t;

#endif
