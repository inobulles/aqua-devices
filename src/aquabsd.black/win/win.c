// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2024 Aymeric Wibo

#include "win.h"

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

static void global_registry_handler(void* data, struct wl_registry* registry, uint32_t name, char const* interface, uint32_t version) {
	(void) version;

	win_t* const win = data;

	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		win->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
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

	LOG_VERBOSE("Got registry event for %s", interface);
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

static void destroy_fb(win_t* win) {
	if (win->shm_fd >= 0) {
		close(win->shm_fd);
		win->shm_fd = -1;
	}

	if (win->fb) {
		munmap(win->fb, win->width * win->height * 4);
		win->fb = NULL;
	}

	if (win->shm_pool) {
		wl_shm_pool_destroy(win->shm_pool);
		win->shm_pool = NULL;
	}
}

static void buffer_release_handler(void* data, struct wl_buffer* buffer) {
	(void) data;

	wl_buffer_destroy(buffer);
	LOG_VERBOSE("Buffer released");
}

static struct wl_buffer_listener const buffer_listener = {
	.release = buffer_release_handler,
};

static void xdg_surface_configure_handler(void* data, struct xdg_surface* xdg_surface, uint32_t serial) {
	(void) xdg_surface;

	win_t* const win = data;

	LOG_VERBOSE("Configuring window XDG surface");
	xdg_surface_ack_configure(win->xdg_surface, serial);

	if (!win->has_fb) {
		goto done;
	}

	LOG_VERBOSE("Destroying previous framebuffer (if there is anything to clean up)");

	destroy_fb(win);

	LOG_VERBOSE("Creating framebuffer");

	size_t const stride = win->width * 4;
	size_t const size = win->height * stride;

	char name[] = "/tmp/aquabsd.black.win.shm-XXXXXXX";
	mktemp(name);

	LOG_VERBOSE("Creating SHM file (%s)", name);

	win->shm_fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);

	if (win->shm_fd < 0) {
		LOG_ERROR("Failed to create SHM file");
		return;
	}

	unlink(name);

	LOG_VERBOSE("Truncating SHM file to %zu bytes", size);

	// XXX the code example in the Wayland Book loops this in case of EINTR - how useful is that actually?

	if (ftruncate(win->shm_fd, size) < 0) {
		LOG_ERROR("Failed to truncate SHM file");
		goto err_fb;
	}

	LOG_VERBOSE("Mapping SHM file to memory");

	win->fb = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, win->shm_fd, 0);

	if (win->fb == MAP_FAILED) {
		LOG_ERROR("Failed to map SHM file to memory");
		goto err_fb;
	}

	LOG_VERBOSE("Creating SHM pool");

	win->shm_pool = wl_shm_create_pool(win->shm, win->shm_fd, size);

	if (win->shm_pool == NULL) {
		LOG_ERROR("Failed to create SHM pool");
		goto err_fb;
	}

	LOG_VERBOSE("Creating frame buffer");
	win->buffer = wl_shm_pool_create_buffer(win->shm_pool, 0, win->width, win->height, stride, WL_SHM_FORMAT_ARGB8888);

	if (win->buffer == NULL) {
		LOG_ERROR("Failed to create frame buffer");
		goto err_fb;
	}

	LOG_VERBOSE("Cleaning up");

	wl_shm_pool_destroy(win->shm_pool);
	win->shm_pool = NULL;

	close(win->shm_fd);
	win->shm_fd = -1;

	LOG_VERBOSE("Adding listener to frame buffer");

	wl_buffer_add_listener(win->buffer, &buffer_listener, win);

	LOG_VERBOSE("Attaching frame buffer to window surface");

	wl_surface_attach(win->surface, win->buffer, 0, 0);
	wl_surface_commit(win->surface);

	goto done;

err_fb:

	destroy_fb(win);

done:

	call_cb(win, WIN_CB_KIND_RESIZE);
}

static struct xdg_surface_listener const xdg_surface_listener = {
	.configure = xdg_surface_configure_handler,
};

win_t* win_create(size_t width, size_t height, bool has_fb) {
	LOG_INFO("Creating window with desired initial size %zux%zu (with%s a framebuffer)", width, height, has_fb ? "" : "out");

	win_t* const win = calloc(1, sizeof *win);
	win->shm_fd = -1;

	if (win == NULL) {
		LOG_FATAL("calloc failed");
		return NULL;
	}

	win->width = width;
	win->height = height;
	win->has_fb = has_fb;

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

	LOG_VERBOSE("Commit window surface");
	wl_surface_commit(win->surface);

	LOG_SUCCESS("Window created");

	return win;
}

void win_destroy(win_t* win) {
	if (win->display) {
		wl_display_disconnect(win->display);
	}

	if (win->registry) {
		wl_registry_destroy(win->registry);
	}

	if (win->surface) {
		wl_surface_destroy(win->surface);
	}

	if (win->compositor) {
		wl_compositor_destroy(win->compositor);
	}

	if (win->shm) {
		wl_shm_destroy(win->shm);
	}

	if (win->xdg_wm_base) {
		xdg_wm_base_destroy(win->xdg_wm_base);
	}

	if (win->xdg_surface) {
		xdg_surface_destroy(win->xdg_surface);
	}

	if (win->xdg_toplevel) {
		xdg_toplevel_destroy(win->xdg_toplevel);
	}

	destroy_fb(win);
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
	LOG_INFO("Start window loop")

	while (wl_display_dispatch(win->display) >= 0) {
		if (call_cb(win, WIN_CB_KIND_DRAW) == 1) {
			// TODO close window
		}
	}

	return 0;
}

uint8_t* win_get_fb(win_t* win) {
	if (!win->has_fb) {
		LOG_ERROR("Window was not created with a framebuffer - did you mean to create the window with has_fb = true?");
		return NULL;
	}

	if (win->fb == NULL) {
		LOG_WARN("No frame buffer yet - waiting for XDG surface configure event");
	}

	return win->fb;
}
