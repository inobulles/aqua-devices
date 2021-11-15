#include <stdio.h>
#include <stdlib.h>

#define FATAL_ERROR(...) fprintf(stderr, "[aquabsd.alps.ogl] FATAL ERROR "__VA_ARGS__); delete_context(context); return NULL;
#define WARNING(...) fprintf(stderr, "[aquabsd.alps.ogl] WARNING "__VA_ARGS__);

dynamic int delete_context(context_t* context) {
	if (context->egl_context) eglDestroyContext(context->egl_display, context->egl_context);
	if (context->egl_surface) eglDestroySurface(context->egl_display, context->egl_surface);

	if (context->display) XCloseDisplay(context->display);

	free(context);

	return 0;
}

// 'create_generic_context' returns 'context_t*' to retain compatibility with the 'FATAL_ERROR' macro
// yeah it's a bit hacky, I know

static context_t* create_generic_context(context_t* context) {
	// setup EGL
	// TODO take a look at how I'm meant to enable/disable vsync
	// TODO ALSO still maintain a GLX backend as it seems to be significantly faster on some platforms

	if (!eglBindAPI(EGL_OPENGL_API)) {
		FATAL_ERROR("Failed to bind EGL API\n")
	}

	context->egl_display = eglGetDisplay(context->display);
	
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

	context->egl_surface = eglCreateWindowSurface(context->egl_display, config, context->window, surface_attribs);

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

	return context;
}

// TODO perhaps separate this stuff into a separate device?

dynamic context_t* create_window(unsigned x_res, unsigned y_res) {
	context_t* context = calloc(1, sizeof *context);

	context->x_res = x_res;
	context->y_res = y_res;

	// get connection to X server

	context->display = XOpenDisplay(NULL /* default to 'DISPLAY' environment variable */);

	if (!context->display) {
		FATAL_ERROR("Failed to open X display\n")
	}

	context->default_screen = DefaultScreen(context->display);
	context->connection = XGetXCBConnection(context->display);

	if (!context->connection) {
		FATAL_ERROR("Failed to get XCB connection from X display\n")
	}

	XSetEventQueueOwner(context->display, XCBOwnsEventQueue);
	
	xcb_screen_iterator_t it = xcb_setup_roots_iterator(xcb_get_setup(context->connection));
	for (int i = context->default_screen; it.rem && i > 0; i--, xcb_screen_next(&it));
	
	context->screen = it.data;

	// create window
	
	const uint32_t window_attribs[] = {
		XCB_EVENT_MASK_EXPOSURE,
	};

	context->window = xcb_generate_id(context->connection);

	xcb_create_window(
		context->connection, XCB_COPY_FROM_PARENT, context->window, context->screen->root,
		0, 0, context->x_res, context->y_res, 0, // window geometry
		XCB_WINDOW_CLASS_INPUT_OUTPUT, context->screen->root_visual,
		XCB_CW_EVENT_MASK, window_attribs);
	
	xcb_map_window(context->connection, context->window);

	// setup 'WM_DELETE_WINDOW' protocol (yes this is dumb, thank you XCB & X11)

	xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom(context->connection, 1, 12 /* strlen("WM_PROTOCOLS") */, "WM_PROTOCOLS");
	xcb_atom_t wm_protocols_atom = xcb_intern_atom_reply(context->connection, wm_protocols_cookie, 0)->atom;

	xcb_intern_atom_cookie_t wm_delete_window_cookie = xcb_intern_atom(context->connection, 0, 16 /* strlen("WM_DELETE_WINDOW") */, "WM_DELETE_WINDOW");
	context->wm_delete_window_atom = xcb_intern_atom_reply(context->connection, wm_delete_window_cookie, 0)->atom;

	xcb_icccm_set_wm_protocols(context->connection, context->window, wm_protocols_atom, 1, &context->wm_delete_window_atom);

	// add caption to window

	#define CAPTION "TODO window captions for aquabsd.alps.ogl"
	xcb_change_property(context->connection, XCB_PROP_MODE_REPLACE, context->window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, sizeof(CAPTION), CAPTION);

	// set sensible minimum and maximum sizes for the window

	xcb_size_hints_t hints;

	xcb_icccm_size_hints_set_min_size(&hints, 320, 200);
	// no maximum size

	xcb_icccm_set_wm_size_hints(context->connection, context->window, XCB_ATOM_WM_NORMAL_HINTS, &hints);

	// create EGL context

	window_t* window = calloc(1, sizeof *window);
	context->backend.window = window;

	if (create_generic_context(context) != context) {
		return NULL;
	}

	return context;
}

dynamic /* wm_t */ int create_wm(void) {
	return -1; // TODO
}

dynamic int register_cb(context_t* context, cb_t type, uint64_t cb, uint64_t param) {
	if (type >= CB_LEN) {
		fprintf(stderr, "[aquabsd.alps.ogl] Callback type %d doesn't exist\n", type);
		return -1;
	}

	context->cbs[type] = cb;
	context->cb_params[type] = param;

	return 0;
}

static inline int call_cb(context_t* context, cb_t type) {
	if (!context->cbs[type]) {
		return -1;
	}

	uint64_t cb = context->cbs[type];
	uint64_t param = context->cb_params[type];

	return kos_callback(cb, 2, context, param);
}

dynamic int loop(context_t* context) {
	for (xcb_generic_event_t* event; (event = xcb_wait_for_event(context->connection)); free(event)) {
		int type = event->response_type & XCB_EVENT_RESPONSE_TYPE_MASK;

		if (type == XCB_EXPOSE) {
			call_cb(context, CB_DRAW);
			eglSwapBuffers(context->egl_display, context->egl_surface);
		}

		else if (type == XCB_CLIENT_MESSAGE) {
			xcb_client_message_event_t* specific = (void*) event;

			if (specific->data.data32[0] == context->wm_delete_window_atom) {
				return 0;
			}
		}
	}

	return 0; // no more events to process
}

dynamic void* get_function(context_t* context, const char* name) {
	return eglGetProcAddress((const GLubyte*) name);
}

