#include <stdio.h>
#include <stdlib.h>

static void swap(context_t* context) {
	eglSwapBuffers(context->egl_display, context->egl_surface);
}

static inline int call_cb(aquabsd_alps_win_t* win, uint64_t cb, uint64_t cb_param) {
	if (!cb) {
		return -1;
	}

	return kos_callback(cb, 2, (uint64_t) win, cb_param);
}

static int win_cb_draw(aquabsd_alps_win_t* win, void* param, uint64_t cb, uint64_t cb_param) {
	int rv = call_cb(win, cb, cb_param);

	if (rv != 0) {
		return rv;
	}

	context_t* context = param;
	swap(context);

	return 0;
}

dynamic int delete(context_t* context) {
	if (context->egl_context) eglDestroyContext(context->egl_display, context->egl_context);
	if (context->egl_surface) eglDestroySurface(context->egl_display, context->egl_surface);

	free(context);

	return 0;
}

// I'll probably need to make this function a bit more generic when supporting more different targets, but for now this works

#define FATAL_ERROR(...) fprintf(stderr, "[aquabsd.alps.ogl] FATAL ERROR "__VA_ARGS__); delete(context); return NULL;
#define WARNING(...) fprintf(stderr, "[aquabsd.alps.ogl] WARNING "__VA_ARGS__);

static EGLConfig config; // TODO remme

dynamic context_t* create_win_context(aquabsd_alps_win_t* win) {
	context_t* context = calloc(1, sizeof *context);
	context->backend.win = win;

	context->x_res = win->x_res;
	context->y_res = win->y_res;

	// setup EGL

	// TODO take a look at how I'm meant to enable/disable vsync
	// TODO ALSO still maintain a GLX backend as it seems to be significantly faster on some platforms

	if (!eglBindAPI(EGL_OPENGL_API)) {
		FATAL_ERROR("Failed to bind EGL API\n")
	}

	context->egl_display = eglGetDisplay(win->display);

	if (context->egl_display == EGL_NO_DISPLAY) {
		FATAL_ERROR("Failed to get EGL display from X11 display\n")
	}

	__attribute__((unused)) int major;
	__attribute__((unused)) int minor;

	if (!eglInitialize(context->egl_display, &major, &minor)) {
		FATAL_ERROR("Failed to initialize EGL\n")
	}

	const int config_attribs[] = {
		EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
		EGL_BUFFER_SIZE, 32,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_DEPTH_SIZE, 16,

		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,

		EGL_NONE
	};

	// EGLConfig config;
	EGLint config_count;

	if (!eglChooseConfig(context->egl_display, config_attribs, &config, 1, &config_count) || !config_count) {
		FATAL_ERROR("Failed to find a suitable EGL config\n")
	}

	const int context_attribs[] = {
		EGL_NONE
	};

	context->egl_context = eglCreateContext(context->egl_display, config, EGL_NO_CONTEXT, context_attribs);

	if (!context->egl_context) {
		FATAL_ERROR("Failed to create EGL context\n")
	}

	const int surface_attribs[] = {
		EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
		EGL_NONE
	};

	context->egl_surface = eglCreateWindowSurface(context->egl_display, config, win->win, surface_attribs);

	if (!context->egl_surface) {
		FATAL_ERROR("Failed to create EGL surface\n")
	}

	if (!eglMakeCurrent(context->egl_display, context->egl_surface, context->egl_surface, context->egl_context)) {
		FATAL_ERROR("Failed to make the EGL context the current GL context\n")
	}

	EGLint render_buffer;
	
	if (!eglQueryContext(context->egl_display, context->egl_context, EGL_RENDER_BUFFER, &render_buffer) || render_buffer == EGL_SINGLE_BUFFER) {
		WARNING("EGL surface is single buffered\n")
	}

	// register callbacks with window

	if (aquabsd_alps_win_register_dev_cb(win, AQUABSD_ALPS_WIN_CB_DRAW, win_cb_draw, context) < 0) {
		FATAL_ERROR("Failed to register draw callback to window\n")
	}

	return context;
}

dynamic void* get_function(context_t* context, const char* name) {
	return eglGetProcAddress(name);
}

// 'EGL_BIND_TO_TEXTURE_RGB' & 'EGL_BIND_TO_TEXTURE_RGBA' seem to have been deprecated (at least by NVIDIA), as per https://forums.developer.nvidia.com/t/egl-bind-to-texture-rgba-attribute-always-false/58676 (this is not reflected in the Khronos EGL registry for some reason)
// so instead of using 'eglCreatePixmapSurface', we shall use 'eglCreateImage'
// to pass the 'buffer' argument to 'eglCreateImage', we must use the undocumented EGL_KHR_image_pixmap extension (https://www.khronos.org/registry/EGL/extensions/KHR/EGL_KHR_image_base.txt) with the 'EGL_NATIVE_PIXMAP_KHR' target (which also means we must load and use 'eglCreateImageKHR' as our 'eglCreateImage' function instead)
// once we finally have our 'EGLImageKHR', we can't bind it to a 'GL_TEXTURE_2D' target, as that would be too simple, so we must use the OES_EGL_image_external extension (https://www.khronos.org/registry/OpenGL/extensions/OES/OES_EGL_image_external.txt)
// this extension provies the 'GL_TEXTURE_EXTERNAL_OES' target and the new 'glEGLImageTargetTexture2DOES' function

// source files which helped:
// - https://github.com/nemomobile/eglext-tests
// - https://github.com/sabipeople/tegra

// TODO check if 'EGL_KHR_image_pixmap' extension is supported
// TODO add this to public.h

#include <xcb/xcb.h>
#include <xcb/composite.h>

#include <GL/gl.h>

#define GL_TEXTURE_EXTERNAL_OES 0x8D65

static xcb_pixmap_t pixmap = 0;
static EGLImageKHR image;
static GLuint texture;

dynamic int bind_win_tex(context_t* context, aquabsd_alps_win_t* win) {
	PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR = get_function(context, "eglCreateImageKHR");
	PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES = get_function(context, "glEGLImageTargetTexture2DOES");

	void (*_glGenTextures) (GLint, GLuint*) = get_function(context, "glGenTextures");
	void (*_glBindTexture) (GLenum, GLuint) = get_function(context, "glBindTexture");

	aquabsd_alps_win_t* context_win = context->backend.win;

	if (pixmap) {
		goto bind;
	}

	pixmap = xcb_generate_id(context_win->connection);
	xcb_composite_name_window_pixmap(context_win->connection, 0xa0002c, pixmap);

	image = eglCreateImageKHR(context->egl_display, EGL_NO_CONTEXT /* don't pass 'context->egl_context' !!! */, EGL_NATIVE_PIXMAP_KHR, (EGLClientBuffer) (intptr_t) pixmap, NULL);

	GLuint texture;
	_glGenTextures(1, &texture);

bind:

	_glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
	glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, image);

	// you must use the 'GL_OES_EGL_image_external' extension to be able to sample from 'samplerExternalOES' samplers:
	// #extension GL_OES_EGL_image_external : require

	return 0;
}