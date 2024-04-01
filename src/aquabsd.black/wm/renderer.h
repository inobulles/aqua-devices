// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2024 Aymeric Wibo

#pragma once

#include "wm.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

typedef struct {
	struct wlr_renderer wlr_renderer;

	wm_t* wm;
	struct wlr_drm_format_set dmabuf_render_formats;

	// EGL stuff

	EGLDeviceEXT egl_device;
	EGLDisplay egl_display;
	EGLContext egl_context;
} renderer_t;

struct wlr_renderer* renderer_create(wm_t* wm);
// TODO void renderer_destroy(struct wlr_renderer* renderer);
