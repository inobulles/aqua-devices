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

#define AQUABSD_BLACK_WIN_SIGNATURE "AQUABSD_BLACK"

typedef struct {
	// to discriminate between aquabsd.alps.win and aquabsd.black.win window objects

	char aquabsd_black_win_signature[16];

	// preferences

	size_t x_res;
	size_t y_res;
	bool has_fb;

	// wayland stuff

	struct wl_display* display;
	struct wl_registry* registry;
	struct wl_surface* surface;

	// these are objects filled in by registry events

	struct wl_compositor* compositor;
	struct wl_shm* shm;
	struct xdg_wm_base* xdg_wm_base;

	// XDG stuff

	struct xdg_surface* xdg_surface;
	struct xdg_toplevel* xdg_toplevel;

	// if we have a framebuffer

	int shm_fd;
	struct wl_shm_pool* shm_pool;
	struct wl_buffer* buffer;
	uint8_t* fb;

	// callbacks

	uint64_t cbs[WIN_CB_KIND_COUNT];
	uint64_t cb_datas[WIN_CB_KIND_COUNT];
} win_t;

win_t* win_create(size_t x_res, size_t y_res, bool has_fb);
void win_destroy(win_t* win);

int win_register_cb(win_t* win, win_cb_kind_t kind, uint64_t cb, uint64_t data);
int win_loop(win_t* win);

uint8_t* win_get_fb(win_t* win);

size_t win_get_x_res(win_t* win);
size_t win_get_y_res(win_t* win);

int win_set_caption(win_t* win, char const* caption);
