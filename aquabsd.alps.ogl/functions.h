#include <stdio.h>
#include <stdlib.h>

static void swap(context_t* context) {
	// void (*_glFinish) (void) = eglGetProcAddress("glFinish");
	// _glFinish();

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

	EGLConfig config;
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

