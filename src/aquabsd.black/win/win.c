// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2024 Aymeric Wibo

#include "win.h"

#include <stdlib.h>
#include <string.h>

static void global_registry_handler(void* data, struct wl_registry* registry, uint32_t name, char const* interface, uint32_t version) {
	(void) version;

	win_t* const win = data;

	LOG_VERBOSE("Got registry event for %s", interface);

	if (strcmp(interface, "wl_compositor") == 0) {
		win->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
	}

	else if (strcmp(interface, "wl_shell") == 0) {
		win->shell = wl_registry_bind(registry, name, &wl_shell_interface, 1);
	}

	else {
		LOG_WARN("Registry event for %s is unknown", interface);
	}
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

	LOG_VERBOSE("Waiting for compositor and shell to be set");

	wl_display_roundtrip(win->display);

	if (win->compositor == NULL) {
		LOG_ERROR("Failed to find compositor");
		win_destroy(win);
		return NULL;
	}

	if (win->shell == NULL) {
		LOG_ERROR("Failed to find shell");
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

	LOG_VERBOSE("Getting shell surface");

	win->shell_surface = wl_shell_get_shell_surface(win->shell, win->surface);

	if (win->shell_surface == NULL) {
		LOG_ERROR("Failed to get shell surface");
		win_destroy(win);
		return NULL;
	}

	LOG_VERBOSE("Making shell surface top level");

	wl_shell_surface_set_toplevel(win->shell_surface);

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

	if (win->shell) {
		wl_shell_destroy(win->shell);
	}

	if (win->surface) {
		wl_surface_destroy(win->surface);
	}

	if (win->shell_surface) {
		wl_shell_surface_destroy(win->shell_surface);
	}

	free(win);
}
