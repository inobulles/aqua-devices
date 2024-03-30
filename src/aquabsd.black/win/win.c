// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2024 Aymeric Wibo

#include "win.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>

static int call_cb(win_t* win, win_cb_kind_t kind) {
	uint64_t cb = win->cbs[kind];
	uint64_t param = win->cb_datas[kind];

	if (!cb) {
		return -1;
	}

	return kos_callback(cb, 2, (uint64_t) win, param);
}

static void xdg_ping_handler(void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial) {
	(void) data;
	(void) xdg_wm_base;

	LOG_VERBOSE("Got XDG ping event with serial %u, ponging", serial);
	xdg_wm_base_pong(xdg_wm_base, serial);
}

static struct xdg_wm_base_listener const xdg_wm_base_listener = {
	.ping = xdg_ping_handler,
};

// XXX we use compositor version 4 to get access to wl_surface_damage_buffer (wl_surface_damage is "effectively" deprecated)
#define COMPOSITOR_VERSION 4

static void global_registry_handler(void* data, struct wl_registry* registry, uint32_t name, char const* interface, uint32_t version) {
	(void) version;

	win_t* const win = data;

	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		if (version < COMPOSITOR_VERSION) {
			LOG_ERROR("Minium required compositor version is %d (version is %u)", COMPOSITOR_VERSION, version);
		}

		else {
			win->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, COMPOSITOR_VERSION);
		}
	}

	else if (strcmp(interface, wl_shm_interface.name) == 0) {
		win->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	}

	else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		win->xdg_wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);

		LOG_VERBOSE("Adding listener to XDG WM base");
		xdg_wm_base_add_listener(win->xdg_wm_base, &xdg_wm_base_listener, win);
	}

	else {
		LOG_WARN("Registry event for %s is unknown", interface);
		return;
	}

	// XXX not ideal, but putting this after to not pollute the log

	LOG_VERBOSE("Got registry event for %s (version %u)", interface, version);
}

static void global_registry_remove_handler(void* data, struct wl_registry* registry, uint32_t name) {
	(void) data;
	(void) registry;

	LOG_VERBOSE("Got registry remove event for %u", name);
}

static struct wl_registry_listener const registry_listener = {
	.global = global_registry_handler,
	.global_remove = global_registry_remove_handler,
};

static void buffer_release_handler(void* data, struct wl_buffer* buffer) {
	(void) data;

	wl_buffer_destroy(buffer);
	LOG_VERBOSE("Buffer released");
}

static struct wl_buffer_listener const buffer_listener = {
	.release = buffer_release_handler,
};

static int draw_frame(win_t* win, struct wl_buffer** buffer_ref) {
	if (!win->has_fb) {
		return 0;
	}

	int rv = -1;

	LOG_VERBOSE("Creating framebuffer");

	size_t const stride = win->x_res * 4;
	size_t const size = win->y_res * stride;

	char name[] = "/tmp/aquabsd.black.win.shm-XXXXXXX";
	mktemp(name);

	LOG_VERBOSE("Creating SHM file (%s)", name);

	int const fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);

	if (fd < 0) {
		LOG_ERROR("Failed to create SHM file");
		goto err_shm_open;
	}

	unlink(name);

	LOG_VERBOSE("Truncating SHM file to %zu bytes", size);

	// XXX the code example in the Wayland Book loops this in case of EINTR - how useful is that actually?

	if (ftruncate(fd, size) < 0) {
		LOG_ERROR("Failed to truncate SHM file");
		goto err_ftruncate;
	}

	LOG_VERBOSE("Mapping SHM file to memory");

	win->fb = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (win->fb == MAP_FAILED) {
		LOG_ERROR("Failed to map SHM file to memory");
		goto err_mmap;
	}

	LOG_VERBOSE("Creating SHM pool");

	struct wl_shm_pool* const pool = wl_shm_create_pool(win->shm, fd, size);

	if (pool == NULL) {
		LOG_ERROR("Failed to create SHM pool");
		goto err_wl_shm_create_pool;
	}

	LOG_VERBOSE("Creating frame buffer");

	struct wl_buffer* const buffer = wl_shm_pool_create_buffer(pool, 0, win->x_res, win->y_res, stride, WL_SHM_FORMAT_ARGB8888);
	*buffer_ref = buffer;

	if (buffer == NULL) {
		LOG_ERROR("Failed to create frame buffer");
		goto err_wl_shm_pool_create_buffer;
	}

	LOG_VERBOSE("Adding listener to frame buffer");
	wl_buffer_add_listener(buffer, &buffer_listener, win);

	LOG_VERBOSE("Calling draw callback");

	if (call_cb(win, WIN_CB_KIND_DRAW)) {
		LOG_INFO("Draw callback requested window to close");
		win->should_close = true;
	}

	rv = 0;

err_wl_shm_pool_create_buffer:

	wl_shm_pool_destroy(pool);

err_wl_shm_create_pool:

	munmap(win->fb, size);
	win->fb = NULL;

err_mmap:
err_ftruncate:

	close(fd);

err_shm_open:

	return rv;
}

static void xdg_surface_configure_handler(void* data, struct xdg_surface* xdg_surface, uint32_t serial) {
	(void) xdg_surface;

	win_t* const win = data;

	LOG_VERBOSE("Configuring window XDG surface");
	xdg_surface_ack_configure(win->xdg_surface, serial);

	if (win->custom_presenter) {
		return;
	}

	struct wl_buffer* buffer = NULL;

	if (draw_frame(win, &buffer) < 0) {
		LOG_ERROR("Failed to draw frame");
		win->should_close = true;
	}

	LOG_VERBOSE("Submitting frame to window surface");

	wl_surface_attach(win->surface, buffer, 0, 0);
	wl_surface_commit(win->surface);
}

static struct xdg_surface_listener const xdg_surface_listener = {
	.configure = xdg_surface_configure_handler,
};

static void xdg_toplevel_configure_handler(void* data, struct xdg_toplevel* xdg_toplevel, int32_t width, int32_t height, struct wl_array* states) {
	(void) xdg_toplevel;
	(void) states;

	win_t* const win = data;

	LOG_VERBOSE("Configuring window XDG toplevel with size %dx%d", width, height);

	if (width <= 0 || height <= 0) {
		LOG_WARN("Invalid window size %dx%d (the Wayland Book says the \"Compositor is deferring to us\", not exactly sure what is meant by that)", width, height);
		return;
	}

	win->x_res = width;
	win->y_res = height;

	call_cb(win, WIN_CB_KIND_RESIZE);
}

static void xdg_toplevel_close_handler(void* data, struct xdg_toplevel* xdg_toplevel) {
	(void) xdg_toplevel;

	win_t* const win = data;

	LOG_INFO("Window closed");
	win->should_close = true;
}

static struct xdg_toplevel_listener const xdg_toplevel_listener = {
	.configure = xdg_toplevel_configure_handler,
	.close = xdg_toplevel_close_handler,
};

static struct wl_callback_listener const frame_listener;

static void frame_done_handler(void* data, struct wl_callback* cb, uint32_t time) {
	(void) time;

	win_t* const win = data;

	if (win->prev_frame_time) {
		win->dt_ms = time - win->prev_frame_time;
	}

	LOG_VERBOSE("New frame requested (dt = %u ms)", win->dt_ms);

	assert(cb == win->frame_cb);
	wl_callback_destroy(cb);

	LOG_VERBOSE("Requesting new frame to be drawn to the compositor after this one");

	win->frame_cb = wl_surface_frame(win->surface);
	wl_callback_add_listener(win->frame_cb, &frame_listener, win);

	struct wl_buffer* buffer = NULL;

	if (draw_frame(win, &buffer) < 0) {
		LOG_ERROR("Failed to draw frame");
		win->should_close = true;
		goto done;
	}

	LOG_VERBOSE("Submitting frame to window surface");

	wl_surface_attach(win->surface, buffer, 0, 0);
	wl_surface_damage_buffer(win->surface, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(win->surface);

done:

	win->prev_frame_time = time;
}

static struct wl_callback_listener const frame_listener = {
	.done = frame_done_handler,
};

char* unique; // KOS-set variable

win_t* win_create(size_t x_res, size_t y_res, win_flag_t flags) {
	bool const has_fb = flags & WIN_FLAG_WITH_FB;
	bool const custom_presenter = flags & WIN_FLAG_CUSTOM_PRESENTER;

	LOG_INFO("Creating window with desired initial size %zux%zu (with%s framebuffer, with%s custom presenter)", x_res, y_res, has_fb ? "" : "out", custom_presenter ? "" : "out");

	win_t* const win = calloc(1, sizeof *win);
	strcpy(win->aquabsd_black_win_signature, AQUABSD_BLACK_WIN_SIGNATURE);

	if (win == NULL) {
		LOG_FATAL("calloc failed");
		return NULL;
	}

	win->x_res = x_res;
	win->y_res = y_res;
	win->has_fb = has_fb;
	win->custom_presenter = custom_presenter;

	LOG_VERBOSE("Connecting to Wayland display");

	win->display = wl_display_connect(NULL);

	if (win->display == NULL) {
		LOG_ERROR("Failed to connect to Wayland display");
		win_destroy(win);
		return NULL;
	}

	LOG_VERBOSE("Getting registry and registering listener");

	win->registry = wl_display_get_registry(win->display);
	wl_registry_add_listener(win->registry, &registry_listener, win);

	LOG_VERBOSE("Waiting for compositor and XDG WM base to be set");

	wl_display_roundtrip(win->display);

	if (win->compositor == NULL) {
		LOG_ERROR("Failed to find compositor");
		win_destroy(win);
		return NULL;
	}

	if (win->xdg_wm_base == NULL) {
		LOG_ERROR("Failed to find XDG WM base");
		win_destroy(win);
		return NULL;
	}

	LOG_VERBOSE("Creating window surface");

	win->surface = wl_compositor_create_surface(win->compositor);

	if (win->surface == NULL) {
		LOG_ERROR("Failed to create window surface");
		win_destroy(win);
		return NULL;
	}

	LOG_VERBOSE("Creating window XDG surface");

	win->xdg_surface = xdg_wm_base_get_xdg_surface(win->xdg_wm_base, win->surface);

	if (win->xdg_surface == NULL) {
		LOG_ERROR("Failed to create window XDG surface");
		win_destroy(win);
		return NULL;
	}

	LOG_VERBOSE("Adding listener to window XDG surface");
	xdg_surface_add_listener(win->xdg_surface, &xdg_surface_listener, win);

	LOG_VERBOSE("Getting window XDG toplevel");
	win->xdg_toplevel = xdg_surface_get_toplevel(win->xdg_surface);

	if (win->xdg_toplevel == NULL) {
		LOG_ERROR("Failed to get window XDG toplevel");
		win_destroy(win);
		return NULL;
	}

	LOG_VERBOSE("Adding listener to window XDG toplevel");
	xdg_toplevel_add_listener(win->xdg_toplevel, &xdg_toplevel_listener, win);

	if (unique == NULL) {
		LOG_WARN("Unique variable not set");
	}

	else {
		LOG_VERBOSE("Setting window XDG toplevel's ID to unique value (%s)", unique);
		xdg_toplevel_set_app_id(win->xdg_toplevel, unique);
	}

	LOG_VERBOSE("Commit window surface");
	wl_surface_commit(win->surface);

	if (!win->custom_presenter) {
		LOG_VERBOSE("Register a surface frame callback");
		win->frame_cb = wl_surface_frame(win->surface);

		if (win->frame_cb == NULL) {
			LOG_ERROR("Failed to register a surface frame callback");
			win_destroy(win);
			return NULL;
		}

		wl_callback_add_listener(win->frame_cb, &frame_listener, win);
	}

	LOG_SUCCESS("Window created");

	return win;
}

void win_destroy(win_t* win) {
	// wayland stuff

	if (win->display) {
		wl_display_disconnect(win->display);
	}

	if (win->registry) {
		wl_registry_destroy(win->registry);
	}

	if (win->surface) {
		wl_surface_destroy(win->surface);
	}

	if (win->frame_cb) {
		wl_callback_destroy(win->frame_cb);
	}

	// objects filled in by registry events

	if (win->compositor) {
		wl_compositor_destroy(win->compositor);
	}

	if (win->shm) {
		wl_shm_destroy(win->shm);
	}

	if (win->xdg_wm_base) {
		xdg_wm_base_destroy(win->xdg_wm_base);
	}

	// XDG stuff

	if (win->xdg_surface) {
		xdg_surface_destroy(win->xdg_surface);
	}

	if (win->xdg_toplevel) {
		xdg_toplevel_destroy(win->xdg_toplevel);
	}

	free(win);
}

int win_register_cb(win_t* win, win_cb_kind_t kind, uint64_t cb, uint64_t data) {
	if (kind >= WIN_CB_KIND_COUNT) {
		LOG_ERROR("Callback kind %d doesn't exist", kind);
		return -1;
	}

	win->cbs[kind] = cb;
	win->cb_datas[kind] = data;

	return 0;
}

int win_loop(win_t* win) {
	LOG_INFO("Start window loop");

	while (!win->should_close) {
		if (wl_display_dispatch(win->display) < 0) {
			LOG_ERROR("Dispatch failure");
			return -1;
		}

		if (win->custom_presenter && call_cb(win, WIN_CB_KIND_DRAW)) {
			win->should_close = true;
		}
	}

	LOG_SUCCESS("Window loop ended");

	return 0;
}

uint8_t* win_get_fb(win_t* win) {
	if (!win->has_fb) {
		LOG_ERROR("Window was not created with a framebuffer - did you mean to create the window with WIN_FLAG_WITH_FB?");
		return NULL;
	}

	if (win->fb == NULL) {
		LOG_WARN("No frame buffer yet - waiting for XDG surface configure event");
	}

	return win->fb;
}

uint32_t win_get_dt_ms(win_t* win) {
	if (win->custom_presenter) {
		LOG_ERROR("Window was created with a custom presenter, the frame time is not available");
	}

	return win->dt_ms;
}

size_t win_get_x_res(win_t* win) {
	return win->x_res;
}

size_t win_get_y_res(win_t* win) {
	return win->y_res;
}

int win_set_caption(win_t* win, char const* caption) {
	xdg_toplevel_set_title(win->xdg_toplevel, caption);
	return 0;
}
