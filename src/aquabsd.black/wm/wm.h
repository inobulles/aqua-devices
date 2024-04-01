// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2024 Aymeric Wibo

#pragma once

#include "common.h"

#include <wayland-client.h>
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

#include <aquabsd.black/win/win.h>

typedef enum {
	WM_FLAG_NONE = 0,
	WM_FLAG_POPULATE_DRM_FD = 1 << 0,
} wm_flag_t;

typedef enum {
	WM_CB_ADD_WINDOW,
	WM_CB_REM_WINDOW,
	WM_CB_COUNT,
} wm_cb_kind_t;

typedef struct {
	// DRM stuff

	int drm_fd;
	bool own_drm_fd;

	// wayland stuff

	struct wl_display* display;
	struct wl_event_loop* event_loop;
	struct wl_compositor* compositor;

	// surface stuff

	struct wl_listener new_surface;
	win_t win; // TODO have no clue how this is supposed to work

	size_t x_res;
	size_t y_res;

	// wlroots stuff

	struct wlr_backend* backend;
	struct wlr_renderer* wlr_renderer;
	struct wlr_allocator* allocator;

	// output layouts

	struct wlr_output_layout* output_layout;
	struct wl_list outputs;
	struct wl_listener new_output;

	// input methods

	struct wl_list inputs;
	struct wl_listener new_input;

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
	struct wl_list keyboards;

	// app client callbacks

	uint64_t cbs[WM_CB_COUNT];
	uint64_t cb_datas[WM_CB_COUNT];
} wm_t;

wm_t* wm_create(wm_flag_t flags);
void wm_destroy(wm_t* wm);

int wm_register_cb(wm_t* wm, wm_cb_kind_t kind, uint64_t cb, uint64_t data);
int wm_loop(wm_t* wm);

size_t wm_get_x_res(wm_t* wm);
size_t wm_get_y_res(wm_t* wm);

// TODO should we refer to windows by struct wlr_xdg_toplevel* or by win_t*?

size_t wm_get_win_x_res(wm_t* wm, struct wlr_xdg_toplevel* toplevel);
size_t wm_get_win_y_res(wm_t* wm, struct wlr_xdg_toplevel* toplevel);
uint8_t* wm_get_win_pixels(wm_t* wm, struct wlr_xdg_toplevel* toplevel);
