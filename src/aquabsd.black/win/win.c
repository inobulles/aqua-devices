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

win_t* win_create(size_t width, size_t height) {
	LOG_INFO("Creating window with size %lux%lu", width, height);

	win_t* const win = calloc(1, sizeof *win);

	if (win == NULL) {
		LOG_FATAL("calloc failed");
		return NULL;
	}

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

	if (win->compositor) {
		wl_compositor_destroy(win->compositor);
	}

	if (win->surface) {
		wl_surface_destroy(win->surface);
	}

	free(win);
}
