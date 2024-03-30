// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2024 Aymeric Wibo

#pragma once

#include "common.h"

#include <wayland-server-core.h>

#define WLR_USE_UNSTABLE 1 // TODO why does the header force me to define this?
#include <wlr/backend.h>
#include <wlr/util/log.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_cursor.h>

typedef struct {
	struct wl_display* display;
	struct wl_event_loop* event_loop;
	struct wlr_backend* backend;
	struct wlr_renderer* renderer;
	struct wlr_allocator* allocator;

	// output layouts

	struct wlr_output_layout* output_layout;
	struct wl_list outputs;
	struct wl_listener new_output;

	// scene graph

	struct wlr_scene* scene;
	struct wlr_scene_output_layout* scene_layout;

	// XDG-shell stuff

	struct wlr_xdg_shell* xdg_shell;
	struct wl_listener new_xdg_toplevel;
	struct wl_listener new_xdg_popup;
	struct wl_list toplevels;

	// cursor stuff

	struct wlr_cursor* cursor;
	struct wl_listener cursor_motion;
	struct wl_listener cursor_motion_absolute;
	struct wl_listener cursor_button;
	struct wl_listener cursor_axis;
	struct wl_listener cursor_frame;

	// seat stuff

	struct wlr_seat* seat;
	struct wl_listener new_input;
	struct wl_list keyboards;
} wm_t;

wm_t* wm_create(void);
void wm_destroy(wm_t* wm);

int wm_loop(wm_t* wm);
