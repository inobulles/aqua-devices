#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <umber.h>
#define UMBER_COMPONENT "aquabsd.alps.ogl"

static inline int call_cb(aquabsd_alps_win_t* win, uint64_t cb, uint64_t cb_param, double dt) {
	if (!cb) {
		return -1;
	}

	uint64_t udt = ((union { double _; uint64_t u; }) dt).u;

	return kos_callback(cb, 3, (uint64_t) win, cb_param, udt);
}

static int win_cb_draw(aquabsd_alps_win_t* win, void* param, uint64_t cb, uint64_t cb_param) {
	context_t* context = param;
	double dt = context->target_ftime;

	if (context->ftime) {
		dt = aquabsd_alps_ftime_draw(context->ftime);
	}

	int rv = call_cb(win, cb, cb_param, dt);

	if (rv > 0) {
		return rv;
	}

	if (context->ftime) {
		aquabsd_alps_ftime_swap(context->ftime);
	}

	eglSwapBuffers(context->egl_display, context->egl_surface);

	if (context->ftime) {
		aquabsd_alps_ftime_done(context->ftime);
	}

	return 0;
}

static int win_cb_resize(aquabsd_alps_win_t* win, void* param, uint64_t cb, uint64_t cb_param) {
	int rv = call_cb(win, cb, cb_param, 0);

	if (rv > 0) {
		return rv;
	}

	context_t* context = param;

	context->x_res = win->x_res;
	context->y_res = win->y_res;

	return 0;
}

dynamic int delete(context_t* context) {
	LOG_VERBOSE("%p: Delete context (including EGL context and EGL surface)")

	if (context->ftime) {
		aquabsd_alps_ftime_delete(context->ftime);
	}

	if (context->egl_context) {
		eglDestroyContext(context->egl_display, context->egl_context);
	}

	if (context->egl_surface) {
		eglDestroySurface(context->egl_display, context->egl_surface);
	}

	free(context);
	return 0;
}

// I'll probably need to make this function a bit more generic when supporting more different targets, but for now this works

#define FATAL_ERROR(...) \
	LOG_FATAL(__VA_ARGS__) \
	delete(context); \
	\
	return NULL;

static const char* egl_error_str(void) {
	EGLint error = eglGetError();

	#define ERROR_CASE(error) \
		case error: return #error;

	switch (error) {
		ERROR_CASE(EGL_SUCCESS            )
		ERROR_CASE(EGL_NOT_INITIALIZED    )
		ERROR_CASE(EGL_BAD_ACCESS         )
		ERROR_CASE(EGL_BAD_ALLOC          )
		ERROR_CASE(EGL_BAD_ATTRIBUTE      )
		ERROR_CASE(EGL_BAD_CONTEXT        )
		ERROR_CASE(EGL_BAD_CONFIG         )
		ERROR_CASE(EGL_BAD_CURRENT_SURFACE)
		ERROR_CASE(EGL_BAD_DISPLAY        )
		ERROR_CASE(EGL_BAD_SURFACE        )
		ERROR_CASE(EGL_BAD_MATCH          )
		ERROR_CASE(EGL_BAD_PARAMETER      )
		ERROR_CASE(EGL_BAD_NATIVE_PIXMAP  )
		ERROR_CASE(EGL_BAD_NATIVE_WINDOW  )
		ERROR_CASE(EGL_CONTEXT_LOST       )

	default:
		return "unknown EGL error; consider setting 'EGL_LOG_LEVEL=debug'";
	}

	#undef ERROR_CASE
}

dynamic context_t* create_win_context(aquabsd_alps_win_t* win) {
	LOG_INFO("Create EGL context for window %p", win)

	context_t* context = calloc(1, sizeof *context);

	context->target_ftime = 1. / 60; // TODO

	if (ftime_device != (uint64_t) -1) {
		context->ftime = aquabsd_alps_ftime_create(context->target_ftime);
	}

	context->win = win;
	context->draw_win = aquabsd_alps_win_get_draw_win(win);

	context->x_res = win->x_res;
	context->y_res = win->y_res;

	// setup EGL

	// TODO take a look at how I'm meant to enable/disable vsync

	LOG_VERBOSE("Bind EGL API (EGL_OPENGL_ES_API)")

	if (!eglBindAPI(EGL_OPENGL_ES_API)) {
		FATAL_ERROR("Failed to bind EGL API: %s", egl_error_str())
	}

	LOG_VERBOSE("Get EGL display from window's X11 display")

	context->egl_display = eglGetDisplay(win->display);

	if (context->egl_display == EGL_NO_DISPLAY) {
		FATAL_ERROR("Failed to get EGL display from X11 display: %s", egl_error_str())
	}

	LOG_VERBOSE("Initialize EGL")

	int major = 0;
	int minor = 0;

	if (!eglInitialize(context->egl_display, &major, &minor)) {
		FATAL_ERROR("Failed to initialize EGL: %s", egl_error_str())
	}

	LOG_INFO("Initialized EGL (version %d.%d)", major, minor)

	LOG_VERBOSE("Find appropriate EGL configurations")

	const int config_attribs[] = {
		EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
		EGL_BUFFER_SIZE,       32,
		EGL_RED_SIZE,          8,
		EGL_GREEN_SIZE,        8,
		EGL_BLUE_SIZE,         8,
		EGL_ALPHA_SIZE,        8,
		EGL_DEPTH_SIZE,        16,

		EGL_SAMPLE_BUFFERS,    1,
		EGL_SAMPLES,           4,

		EGL_SURFACE_TYPE,      EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE,   EGL_OPENGL_BIT,

		EGL_NONE
	};

	EGLConfig config;
	EGLint config_count;

	if (!eglChooseConfig(context->egl_display, config_attribs, &config, 1, &config_count) || !config_count) {
		FATAL_ERROR("Failed to find a suitable EGL config: %s", egl_error_str())
	}

	const int context_attribs[] = {
		EGL_CONTEXT_MAJOR_VERSION, 3,
		EGL_CONTEXT_MINOR_VERSION, 0,
		EGL_NONE
	};

	LOG_VERBOSE("Create EGL context from the first configuration found (context version %d.%d)", context_attribs[1], context_attribs[3])

	context->egl_context = eglCreateContext(context->egl_display, config, EGL_NO_CONTEXT, context_attribs);

	if (!context->egl_context) {
		FATAL_ERROR("Failed to create EGL context: %s", egl_error_str())
	}

	LOG_VERBOSE("Create EGL surface from window")

	const int surface_attribs[] = {
		EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
		EGL_NONE
	};

	context->egl_surface = eglCreateWindowSurface(context->egl_display, config, context->draw_win, surface_attribs);

	if (!context->egl_surface) {
		FATAL_ERROR("Failed to create EGL surface: %s", egl_error_str())
	}

	LOG_VERBOSE("Make EGL context the current context of the window")

	if (!eglMakeCurrent(context->egl_display, context->egl_surface, context->egl_surface, context->egl_context)) {
		FATAL_ERROR("Failed to make the EGL context the current GL context: %s", egl_error_str())
	}

	LOG_VERBOSE("Query EGL_RENDER_BUFFER from context")

	EGLint render_buffer;

	if (!eglQueryContext(context->egl_display, context->egl_context, EGL_RENDER_BUFFER, &render_buffer) || render_buffer == EGL_SINGLE_BUFFER) {
		LOG_WARN("EGL surface is single buffered (%s)", egl_error_str())
	}

	LOG_VERBOSE("Set swap interval (VSYNC)")

	if (!eglSwapInterval(context->egl_display, 1)) {
		LOG_WARN("Failed to set swap interval: %s", egl_error_str())
	}

	// register callbacks with window

	LOG_VERBOSE("Register draw callback with the window")

	if (aquabsd_alps_win_register_dev_cb(win, AQUABSD_ALPS_WIN_CB_DRAW, win_cb_draw, context) < 0) {
		FATAL_ERROR("Failed to register draw callback to window")
	}

	LOG_VERBOSE("Register resize callback with the window")

	if (aquabsd_alps_win_register_dev_cb(win, AQUABSD_ALPS_WIN_CB_RESIZE, win_cb_resize, context) < 0) {
		FATAL_ERROR("Failed to register resize callback to window")
	}

	LOG_INFO("EGL vendor:        %s", eglQueryString(context->egl_display, EGL_VENDOR))
	LOG_INFO("EGL version:       %s", eglQueryString(context->egl_display, EGL_VERSION))

	const GLubyte* (*glGetString) (GLenum name) = (void*) eglGetProcAddress("glGetString");

	LOG_INFO("OpenGL vendor:     %s", glGetString(GL_VENDOR))
	LOG_INFO("OpenGL renderer:   %s", glGetString(GL_RENDERER))
	LOG_INFO("OpenGL version:    %s", glGetString(GL_VERSION))
	LOG_INFO("OpenGL SL version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION))

	LOG_SUCCESS("EGL context created: %p", context)

	return context;
}

dynamic void* get_function(context_t* context, const char* name) {
	LOG_VERBOSE("%p: Get function '%s'", context, name)

	return eglGetProcAddress(name);
}

// quick tip: 'export EGL_LOG_LEVEL=debug' before debugging EGL apps
// 'EGL_BIND_TO_TEXTURE_RGB' & 'EGL_BIND_TO_TEXTURE_RGBA' seem to have been deprecated (at least by NVIDIA), as per https://forums.developer.nvidia.com/t/egl-bind-to-texture-rgba-attribute-always-false/58676 (this is not reflected in the Khronos EGL registry for some reason)
// so instead of using 'eglCreatePixmapSurface', we shall use 'eglCreateImage'
// to pass the 'buffer' argument to 'eglCreateImage', we must use the undocumented EGL_KHR_image_pixmap extension (https://www.khronos.org/registry/EGL/extensions/KHR/EGL_KHR_image_base.txt) with the 'EGL_NATIVE_PIXMAP_KHR' target (which also means we must load and use 'eglCreateImageKHR' as our 'eglCreateImage' function instead)
// once we finally have our 'EGLImageKHR', we can't bind it to a 'GL_TEXTURE_2D' target, as that would be too simple, so we must use the OES_EGL_image_external extension (https://www.khronos.org/registry/OpenGL/extensions/OES/OES_EGL_image_external.txt)
// this extension provides the 'GL_TEXTURE_EXTERNAL_OES' target and the new 'glEGLImageTargetTexture2DOES' function
// also, in the MESA GL driver, it seems the OES_EGL_image_external extension is only supported when using the OpenGL ES API, so don't forget to use that and set the context version appropriately when binding the EGL API (https://github.com/mesa3d/mesa/blob/43dd023bd1eb23a5cdb1470c6a30595c3fbf319a/src/mesa/main/extensions_table.h)
// on some installations, the MESA GL driver fails with 'xcb_dri3_buffer_from_pixmap'. As a temporary fix, try disabling DRI3 with 'export LIBGL_DRI3_DISABLE=1'

// source files which helped:
// - https://github.com/nemomobile/eglext-tests
// - https://github.com/sabipeople/tegra

// TODO check if 'EGL_KHR_image_pixmap' extension is supported
// TODO add this to public.h
// TODO well idk just clean all of this up basically because this is janky as hell

#include <xcb/xcb.h>
#include <xcb/composite.h>

static PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR = NULL;
static void (*glEGLImageTargetTexture2DOES) (GLenum target, GLeglImageOES image) = NULL; // could've used 'PFNGLEGLIMAGETARGETTEXTURE2DOESPROC' from <GL/gl.h> here

static void (*glGenTextures) (GLsizei n, GLuint* textures) = NULL;
static void (*glDeleteTextures) (GLsizei n, GLuint* textures) = NULL;
static void (*glBindTexture) (GLenum target, GLuint texture) = NULL;

dynamic int bind_win_tex(context_t* context, aquabsd_alps_win_t* win) {
	if (!win) { // this only works sometimes
		win = context->win;
	}

	#define CHECK_AND_GET(name) \
		if (!name) { \
			name = get_function(context, #name); \
		}

	CHECK_AND_GET(eglCreateImageKHR)
	CHECK_AND_GET(glEGLImageTargetTexture2DOES)

	CHECK_AND_GET(glGenTextures)
	CHECK_AND_GET(glDeleteTextures)
	CHECK_AND_GET(glBindTexture)

	if (!eglCreateImageKHR || !glEGLImageTargetTexture2DOES || !glGenTextures || !glDeleteTextures || !glBindTexture) {
		return -1; // well... I guess some of the extensions aren't supported then 🤷
	}

	if (win->pixmap_modified) {
		win->pixmap_modified = 0;

		glDeleteTextures(1, &win->egl_texture);
		eglDestroyImage(context->egl_display, win->egl_image);
		xcb_free_pixmap(context->win->connection, win->egl_pixmap);

		win->egl_image = 0;
	}

	xcb_window_t xcb_win = aquabsd_alps_win_get_draw_win(win);

	unsigned nvidia = 1;

	if (win->egl_image/* || win->egl_surface*/) {
		goto bind;
	}

	win->egl_pixmap = xcb_generate_id(context->win->connection);
	xcb_void_cookie_t cookie = xcb_composite_name_window_pixmap_checked(context->win->connection, xcb_win, win->egl_pixmap);

	if (xcb_request_check(context->win->connection, cookie)) {
		LOG_ERROR("Failed to create window pixmap. Do you happen to be running MESA and not as a window manager?")
		return -1;
	}

	if (nvidia) {
		const EGLint attribs[] = {
			EGL_IMAGE_PRESERVED_KHR, EGL_FALSE,
			EGL_NONE
		};

		win->egl_image = eglCreateImageKHR(context->egl_display, EGL_NO_CONTEXT /* don't pass 'context->egl_context' !!! */, EGL_NATIVE_PIXMAP_KHR, (EGLClientBuffer) (intptr_t) win->egl_pixmap, attribs);
		xcb_free_pixmap(context->win->connection, win->egl_pixmap);

		if (win->egl_image == EGL_NO_IMAGE) {
			return -1;
		}

		glGenTextures(1, &win->egl_texture);
	}

	// else {
	// 	win->egl_surface = eglCreatePixmapSurface(context->egl_display, config, win->egl_pixmap, NULL);
	// }

bind:

	if (nvidia) {
		glBindTexture(GL_TEXTURE_EXTERNAL_OES, win->egl_texture);
		glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, win->egl_image);

		// you must use the 'GL_OES_EGL_image_external' extension to be able to sample from 'samplerExternalOES' samplers:
		// #extension GL_OES_EGL_image_external : require
	}

	// else {
	// 	eglBindTexImage(context->egl_display, win->egl_surface, EGL_BACK_BUFFER /* this argument doesn't seem to be correctly documented by Khronos */);
	// }

	return 0;
}
