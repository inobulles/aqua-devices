#if !defined(__AQUABSD_ALPS_OGL)
#define __AQUABSD_ALPS_OGL

#include <aquabsd.alps.win/public.h>

#include <EGL/egl.h>

// from the 'ogl' library (in 'aqua-lib')
// there, there is a generator provided to automatically update it

#include <aquabsd.alps.ogl/gl/gl.h>

typedef enum {
	AQUABSD_ALPS_OGL_CONTEXT_TYPE_WIN,
	AQUABSD_ALPS_OGL_CONTEXT_TYPE_WM,
	AQUABSD_ALPS_OGL_CONTEXT_TYPE_LEN,
} aquabsd_alps_ogl_context_type_t;

typedef struct {
	// backend stuff

	aquabsd_alps_ogl_context_type_t backend_type;

	union { // TODO is this union ever really necessary?
		aquabsd_alps_win_t* win;
	} backend;

	// EGL stuff

	EGLDisplay* egl_display;
	EGLContext* egl_context;
	EGLSurface* egl_surface;
} aquabsd_alps_ogl_context_t;

static aquabsd_alps_ogl_context_t* (*aquabsd_alps_ogl_create_win_context) (aquabsd_alps_win_t* win);
static int (*aquabsd_alps_ogl_delete_context) (aquabsd_alps_ogl_context_t* context);

static void* (*aquabsd_alps_ogl_get_function) (aquabsd_alps_ogl_context_t* context, const char* name);

#endif
