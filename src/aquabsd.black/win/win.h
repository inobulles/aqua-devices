// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2024 Aymeric Wibo

#pragma once

#include "common.h"

#include <wayland-client.h>

typedef struct {
	struct wl_display* display;
	struct wl_registry* registry;

	// these are objects filled in by registry events

	struct wl_compositor* compositor;

	struct wl_surface* surface;
} win_t;

win_t* win_create(size_t width, size_t height);
void win_destroy(win_t* win);
