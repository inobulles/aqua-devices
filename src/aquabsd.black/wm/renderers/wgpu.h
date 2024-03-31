// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2024 Aymeric Wibo

#pragma once

#include <aquabsd.black/wgpu/webgpu.h>

#include <umber.h>
#define UMBER_COMPONENT "aquabsd.black.wm.renderer.wgpu"

typedef struct {
	WGPUInstance instance;
} wm_renderer_wgpu_t;

int wm_create_renderer_wgpu(wm_renderer_wgpu_t* renderer);
void wm_destroy_renderer_wgpu(wm_renderer_wgpu_t* renderer);
