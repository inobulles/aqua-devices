// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2024 Aymeric Wibo

#pragma once

#include "common.h"

#include <wayland-client.h>

#include "xdg-shell-client-protocol.h"

#include <stdbool.h>

typedef enum {
	WIN_CB_KIND_DRAW,
	WIN_CB_KIND_RESIZE,
	WIN_CB_KIND_COUNT,
} win_cb_kind_t;

typedef enum {
	WIN_FLAG_NONE = 0b00,
	WIN_FLAG_WITH_FB = 0b01,
	WIN_FLAG_CUSTOM_PRESENTER = 0b10,
} win_flag_t;

#define AQUABSD_BLACK_WIN_SIGNATURE "AQUABSD_BLACK"

typedef struct {
	// to discriminate between aquabsd.alps.win and aquabsd.black.win window objects

	char aquabsd_black_win_signature[16];

	// preferences and state

	size_t x_res;
	size_t y_res;
	bool has_fb;
	bool custom_presenter;
	bool should_close;
	uint32_t prev_frame_time;
	uint32_t dt_ms;

	// wayland stuff

	struct wl_display* display;
	struct wl_registry* registry;
	struct wl_surface* surface;
	struct wl_callback* frame_cb;

	// these are objects filled in by registry events

	struct wl_compositor* compositor;
	struct wl_shm* shm;
	struct xdg_wm_base* xdg_wm_base;

	// XDG stuff

	struct xdg_surface* xdg_surface;
	struct xdg_toplevel* xdg_toplevel;

	// if we have a framebuffer

	uint8_t* fb;

	// callbacks

	uint64_t cbs[WIN_CB_KIND_COUNT];
	uint64_t cb_datas[WIN_CB_KIND_COUNT];
} win_t;

win_t* win_create(size_t x_res, size_t y_res, win_flag_t flags);
void win_destroy(win_t* win);

int win_register_cb(win_t* win, win_cb_kind_t kind, uint64_t cb, uint64_t data);
int win_loop(win_t* win);

uint8_t* win_get_fb(win_t* win);

uint32_t win_get_dt_ms(win_t* win);

size_t win_get_x_res(win_t* win);
size_t win_get_y_res(win_t* win);

int win_set_caption(win_t* win, char const* caption);
