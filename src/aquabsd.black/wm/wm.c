// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2024 Aymeric Wibo

// a lot of this code is based on the tinywl reference compositor and the "simple.c" example:
// https://gitlab.freedesktop.org/wlroots/wlroots/-/blob/master/tinywl
// https://gitlab.freedesktop.org/wlroots/wlroots/-/blob/master/examples/simple.c

#include "wm.h"
#include "drm.h"
#include "renderer.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int call_cb(wm_t* wm, wm_cb_kind_t kind) {
	uint64_t cb = wm->cbs[kind];
	uint64_t param = wm->cb_datas[kind];

	if (!cb) {
		return -1;
	}

	return kos_callback(cb, 2, (uint64_t) wm, param);
}

static void wlr_log_cb(enum wlr_log_importance importance, char const* fmt, va_list args) {
	char* msg;
	vasprintf(&msg, fmt, args);

	if (msg == NULL) {
		LOG_FATAL("vasprintf failed");
		return;
	}

	// if the importance is anything other than the three here, we consider that necessarily abnormal behaviour, so we log it as a bona fide error
	// a WLR_ERROR could just be a warning, so we log it as a warning, and hope that if it truly is an error, it will be logged elsewhere

	umber_lvl_t lvl = UMBER_LVL_ERROR;

	if (importance == WLR_DEBUG) {
		lvl = UMBER_LVL_VERBOSE;
	}

	else if (importance == WLR_INFO) {
		lvl = UMBER_LVL_INFO;
	}

	else if (importance == WLR_ERROR) {
		lvl = UMBER_LVL_WARN;
	}

	umber_log(lvl, UMBER_COMPONENT, "wlroots", "wlroots", 69, msg);
	free(msg);
}

static bool logging_set_up = false;

static void setup_logging(void) {
	if (logging_set_up) {
		return;
	}

	wlr_log_init(WLR_DEBUG, wlr_log_cb);
	logging_set_up = true;
}

typedef struct {
	wm_t* wm;
	struct wlr_output* output;
	struct wl_listener frame;
	struct wl_listener destroy;
} output_t;

#include <GLES2/gl2.h>

static void output_frame_notify(struct wl_listener* listener, void* data) {
	(void) data;

	output_t* const my_output = wl_container_of(listener, my_output, frame);
	wm_t* const wm = my_output->wm;
	struct wlr_output* const wlr_output = my_output->output;

	struct wlr_output_state state;
	wlr_output_state_init(&state);

	struct wlr_render_pass* pass = wlr_output_begin_render_pass(wlr_output, &state, NULL, NULL);

	call_cb(wm, WM_CB_KIND_DRAW);
	glFlush();

	wlr_render_pass_submit(pass);
	wlr_output_commit_state(wlr_output, &state);
	wlr_output_state_finish(&state);
}

static void output_remove_notify(struct wl_listener* listener, void* data) {
	(void) data;

	output_t* const output = wl_container_of(listener, output, destroy);

	wl_list_remove(&output->frame.link);
	wl_list_remove(&output->destroy.link);

	free(output);
}

static void new_output(struct wl_listener* listener, void* data) {
	struct wlr_output* const output = data;
	wm_t* const wm = wl_container_of(listener, wm, new_output);

	LOG_FATAL("TODO");

	wlr_output_init_render(output, wm->allocator, wm->wlr_renderer);

	output_t* const my_output = calloc(1, sizeof *output);

	my_output->wm = wm;
	my_output->output = output;

	wl_signal_add(&output->events.frame, &my_output->frame);
	my_output->frame.notify = output_frame_notify;

	wl_signal_add(&output->events.destroy, &my_output->destroy);
	my_output->destroy.notify = output_remove_notify;

	struct wlr_output_state state;

	wlr_output_state_init(&state);
	wlr_output_state_set_enabled(&state, true);

	struct wlr_output_mode *mode = wlr_output_preferred_mode(output);

	if (mode != NULL) {
		wlr_output_state_set_mode(&state, mode);
	}

	wlr_output_commit_state(output, &state);
	wlr_output_state_finish(&state);
}

static void new_toplevel(struct wl_listener* listener, void* data) {
	(void) listener;
	(void) data;

	LOG_FATAL("TODO");
}

static void new_popup(struct wl_listener* listener, void* data) {
	(void) listener;
	(void) data;

	LOG_FATAL("TODO");
}

static void cursor_motion(struct wl_listener* listener, void* data) {
	(void) listener;
	(void) data;

	LOG_FATAL("TODO");
}

static void cursor_motion_absolute(struct wl_listener* listener, void* data) {
	(void) listener;
	(void) data;

	LOG_FATAL("TODO");
}

static void cursor_button(struct wl_listener* listener, void* data) {
	(void) listener;
	(void) data;

	LOG_FATAL("TODO");
}

static void cursor_axis(struct wl_listener* listener, void* data) {
	(void) listener;
	(void) data;

	LOG_FATAL("TODO");
}

static void cursor_frame(struct wl_listener* listener, void* data) {
	(void) listener;
	(void) data;

	LOG_FATAL("TODO");
}

static void new_input(struct wl_listener* listener, void* data) {
	(void) listener;
	(void) data;

	LOG_FATAL("TODO");
}

wm_t* wm_create(wm_flag_t flags) {
	LOG_INFO("Creating WM");

	wm_t* const wm = calloc(1, sizeof *wm);

	if (wm == NULL) {
		LOG_FATAL("WM object allocation failed");
		return NULL;
	}

	setup_logging();

#define FAIL(...) \
	do { \
		LOG_ERROR(__VA_ARGS__); \
		wm_destroy(wm); \
		return NULL; \
	} while (0)

	LOG_VERBOSE("Creating the Wayland display");
	wm->display = wl_display_create();

	if (wm->display == NULL) {
		FAIL("Failed to create Wayland display");
	}

	LOG_VERBOSE("Getting the Wayland event loop");
	wm->event_loop = wl_display_get_event_loop(wm->display);

	LOG_VERBOSE("Creating appropriate backend");
	wm->backend = wlr_backend_autocreate(wm->event_loop, NULL);

	if (wm->backend == NULL) {
		FAIL("Failed to create backend");
	}

	wm->drm_fd = -1;

	if (flags & WM_FLAG_POPULATE_DRM_FD) {
		LOG_VERBOSE("Populating DRM fd");

		if (open_preferred_drm_fd(wm, &wm->drm_fd, &wm->own_drm_fd) < 0) {
			FAIL("Failed to populate DRM fd");
		}

		LOG_INFO("Got DRM fd (%d)", wm->drm_fd);
	}

	LOG_VERBOSE("Creating renderer");
	wm->wlr_renderer = renderer_create(wm);

	if (wm->wlr_renderer == NULL) {
		FAIL("Failed to create renderer");
	}

	// don't think this is necessary per se, simple.c doesn't do it at least
	// LOG_VERBOSE("Initializing renderer");

	// if (!wlr_renderer_init_wl_display(wm->wlr_renderer, wm->display)) {
	// 	FAIL("Failed to initialize renderer");
	// }

	LOG_VERBOSE("Creating wlroots allocator");
	wm->allocator = wlr_allocator_autocreate(wm->backend, wm->wlr_renderer);

	if (wm->allocator == NULL) {
		FAIL("Failed to create wlroots allocator");
	}

	LOG_VERBOSE("Add listener for when new outputs are available");

	wl_list_init(&wm->outputs);
	wm->new_output.notify = new_output;
	wl_signal_add(&wm->backend->events.new_output, &wm->new_output);

	LOG_VERBOSE("Add listener for when new input methods are available");

	wl_list_init(&wm->inputs);
	wm->new_input.notify = new_input;
	wl_signal_add(&wm->backend->events.new_input, &wm->new_input);

	LOG_VERBOSE("Start backend (current EGLDisplay: %p)", eglGetCurrentDisplay());

	if (!wlr_backend_start(wm->backend)) {
		// LOG_ERROR("Failed to start backend");
		// return -1;

		FAIL("Failed to start backend");
	}

	LOG_FATAL("TODO the actual compositing part");
	return wm;

	LOG_VERBOSE("Creating compositor (version 5)");
	struct wlr_compositor* const compositor = wlr_compositor_create(wm->display, 5, wm->wlr_renderer);

	if (compositor == NULL) {
		FAIL("Failed to create compositor");
	}

	LOG_VERBOSE("Add listener for when new surfaces are available");

	wm->new_surface.notify = new_toplevel;
	wl_signal_add(&compositor->events.new_surface, &wm->new_surface);

	LOG_VERBOSE("Creating subcompositor");

	if (wlr_subcompositor_create(wm->display) == NULL) {
		FAIL("Failed to create subcompositor");
	}

	LOG_VERBOSE("Creating data device manager");

	if (wlr_data_device_manager_create(wm->display) == NULL) {
		FAIL("Failed to create data device manager");
	}

	LOG_VERBOSE("Creating output layout");
	wm->output_layout = wlr_output_layout_create(wm->display);

	if (wm->output_layout == NULL) {
		FAIL("Failed to create output layout");
	}

	LOG_VERBOSE("Create scene graph");
	wm->scene = wlr_scene_create();

	if (wm->scene == NULL) {
		FAIL("Failed to create scene graph");
	}

	LOG_VERBOSE("Create scene layout");
	wm->scene_layout = wlr_scene_attach_output_layout(wm->scene, wm->output_layout);

	if (wm->scene_layout == NULL) {
		FAIL("Failed to create scene layout");
	}

	LOG_VERBOSE("Set up XDG-shell (version 3)");

	wl_list_init(&wm->toplevels);
	wm->xdg_shell = wlr_xdg_shell_create(wm->display, 3);

	if (wm->xdg_shell == NULL) {
		FAIL("Failed to create XDG-shell");
	}

	LOG_VERBOSE("Add listener for when new XDG toplevels or popups are available");

	wm->new_xdg_toplevel.notify = new_toplevel;
	wl_signal_add(&wm->xdg_shell->events.new_surface, &wm->new_xdg_toplevel);

	wm->new_xdg_popup.notify = new_popup;
	wl_signal_add(&wm->xdg_shell->events.new_popup, &wm->new_xdg_popup);

	LOG_VERBOSE("Create cursor");
	wm->cursor = wlr_cursor_create();

	if (wm->cursor == NULL) {
		FAIL("Failed to create cursor");
	}

	wlr_cursor_attach_output_layout(wm->cursor, wm->output_layout);

	LOG_VERBOSE("Add listeners for cursor events");

	wm->cursor_motion.notify = cursor_motion;
	wl_signal_add(&wm->cursor->events.motion, &wm->cursor_motion);

	wm->cursor_motion_absolute.notify = cursor_motion_absolute;
	wl_signal_add(&wm->cursor->events.motion_absolute, &wm->cursor_motion_absolute);

	wm->cursor_button.notify = cursor_button;
	wl_signal_add(&wm->cursor->events.button, &wm->cursor_button);

	wm->cursor_axis.notify = cursor_axis;
	wl_signal_add(&wm->cursor->events.axis, &wm->cursor_axis);

	wm->cursor_frame.notify = cursor_frame;
	wl_signal_add(&wm->cursor->events.frame, &wm->cursor_frame);

	LOG_VERBOSE("Create new seat");
	wm->seat = wlr_seat_create(wm->display, "seat0");

	if (wm->seat == NULL) {
		FAIL("Failed to create seat");
	}

	LOG_FATAL("STOP HERE FOR NOW");
	return wm;

	LOG_VERBOSE("Start backend");

	if (!wlr_backend_start(wm->backend)) {
		FAIL("Failed to start backend");
	}

	/*
	LOG_VERBOSE("Create surface");
	wm->surface = wl_compositor_create_surface(wm->compositor);

	if (wm->surface == NULL) {
		FAIL("Failed to create surface");
	}

	LOG_VERBOSE("Create XDG surface");
	wm->xdg_surface = wlr_xdg_surface_create(wm->xdg_shell, wm->surface);
	*/

	LOG_VERBOSE("Add UNIX socket to the Wayland display");
	char const* const sock = wl_display_add_socket_auto(wm->display);

	if (sock == NULL) {
		FAIL("Failed to add UNIX socket to Wayland display");
	}

	LOG_INFO("Wayland display is listening on %s", sock);
	LOG_VERBOSE("Start the backend");

	if (!wlr_backend_start(wm->backend)) {
		FAIL("Failed to start backend");
	}

	LOG_VERBOSE("Seting WAYLAND_DISPLAY environment variable to socket");
	setenv("WAYLAND_DISPLAY", sock, true);

	LOG_SUCCESS("WM created");

	return wm;
}

void wm_destroy(wm_t* wm) {
	if (wm->drm_fd >= 0 && wm->own_drm_fd) {
		close(wm->drm_fd);
	}

	if (wm->display != NULL) {
		wl_display_destroy(wm->display);
	}

	if (wm->backend != NULL) {
		wlr_backend_destroy(wm->backend);
	}

	if (wm->allocator != NULL) {
		wlr_allocator_destroy(wm->allocator);
	}

	// TODO alles die hier nog niet gefree'd zijn

	free(wm);
}

int wm_register_cb(wm_t* wm, wm_cb_kind_t kind, uint64_t cb, uint64_t data) {
	if (kind >= WIN_CB_KIND_COUNT) {
		LOG_ERROR("Callback kind %d doesn't exist", kind);
		return -1;
	}

	wm->cbs[kind] = cb;
	wm->cb_datas[kind] = data;

	return 0;
}

#include <EGL/egl.h>

int wm_loop(wm_t* wm) {
	LOG_INFO("Starting WM loop");

	wl_display_run(wm->display);

	LOG_SUCCESS("WM loop ended");

	return 0;
}

size_t wm_get_x_res(wm_t* wm) {
	return wm->x_res;
}

size_t wm_get_y_res(wm_t* wm) {
	return wm->y_res;
}
