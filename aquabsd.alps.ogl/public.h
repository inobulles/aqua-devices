#if !defined(__AQUABSD_ALPS_OGL)
#define __AQUABSD_ALPS_OGL

#include <aquabsd.alps.win/public.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

typedef enum {
	AQUABSD_ALPS_OGL_CONTEXT_TYPE_WIN,
	AQUABSD_ALPS_OGL_CONTEXT_TYPE_LEN,
} aquabsd_alps_ogl_context_type_t;

typedef struct {
	unsigned x_res, y_res;

	// backend stuff

	aquabsd_alps_ogl_context_type_t backend_type;

	aquabsd_alps_win_t* win;
	xcb_window_t draw_win;

	// EGL stuff

	EGLDisplay* egl_display;
	EGLContext* egl_context;
	EGLSurface* egl_surface;
} aquabsd_alps_ogl_context_t;

extern aquabsd_alps_ogl_context_t* (*aquabsd_alps_ogl_create_win_context) (aquabsd_alps_win_t* win);
extern int (*aquabsd_alps_ogl_delete_context) (aquabsd_alps_ogl_context_t* context);

extern void* (*aquabsd_alps_ogl_get_function) (aquabsd_alps_ogl_context_t* context, const char* name);
extern int (*aquabsd_alps_ogl_bind_win_tex) (aquabsd_alps_ogl_context_t* context, aquabsd_alps_win_t* win);

#endif
