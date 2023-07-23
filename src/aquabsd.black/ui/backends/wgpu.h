// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2023 Aymeric Wibo

#pragma once

#include <aquabsd.alps/win/public.h>
#include <aquabsd.black/wgpu/webgpu.h>

typedef struct {
	aquabsd_alps_win_t* win;

	WGPUInstance instance;
	WGPUSurface surface;
	WGPUAdapter adapter;
	WGPUDevice device;
	WGPURenderPipeline render_pipeline;
	WGPUTextureFormat surface_preferred_format;
	WGPUSwapChain swapchain;
	WGPUQueue queue;
} backend_wgpu_t;

backend_wgpu_t* backend_wgpu_create(aquabsd_alps_win_t* win);
void backend_wgpu_destroy(void* wgpu);
