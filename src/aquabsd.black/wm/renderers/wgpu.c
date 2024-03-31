// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2024 Aymeric Wibo

#include "wgpu.h"

#include <string.h>

int wm_create_renderer_wgpu(wm_renderer_wgpu_t* renderer) {
	memset(renderer, 0, sizeof *renderer);

	// an instance is just a shell around WebGPU stuff
	// there's nothing special to do here
	// TODO in fact, this could probably be passed in by the client to make things simpler

	LOG_VERBOSE("Create WebGPU instance");
	renderer->instance = wgpuCreateInstance(NULL);

	if (!renderer->instance) {
		LOG_FATAL("Failed to create WebGPU instance");
		wm_destroy_renderer_wgpu(renderer);
		return -1;
	}

	// this is the part where we have to create the adapter ourselves

	LOG_VERBOSE("Request WebGPU adapter");

	WGPURequestAdapterOptions const request_adapter_options = {
		.compatibleSurface = NULL,
		.backendType = WGPUBackendType_OpenGL,
	};

	return 0;
}

void wm_destroy_renderer_wgpu(wm_renderer_wgpu_t* renderer) {
	if (renderer->instance) {
		wgpuInstanceRelease(renderer->instance);
	}
}
