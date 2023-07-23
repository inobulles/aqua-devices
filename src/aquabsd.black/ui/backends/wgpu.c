// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2023 Aymeric Wibo

#include <backends/wgpu.h>

#include <umber.h>
#define UMBER_COMPONENT "aquabsd.black.ui/wgpu"

static void request_adapter_cb(WGPURequestAdapterStatus status, WGPUAdapter adapter, char const* msg, void* data) {
	backend_wgpu_t* const backend_wgpu = data;

	if (status == WGPURequestAdapterStatus_Success) {
		backend_wgpu->adapter = adapter;
	}

	else {
		LOG_ERROR("%s: status = %#.8x, message = \"%s\"", __func__, status, msg);
	}
}

static void request_device_cb(WGPURequestDeviceStatus status, WGPUDevice device, char const* msg, void* data) {
	backend_wgpu_t* const backend_wgpu = data;

	if (status == WGPURequestAdapterStatus_Success) {
		backend_wgpu->device = device;
	}

	else {
		LOG_ERROR("%s: status = %#.8x, message = \"%s\"", __func__, status, msg);
	}
}

backend_wgpu_t* backend_wgpu_create(aquabsd_alps_win_t* win) {
	backend_wgpu_t* const wgpu = calloc(1, sizeof *wgpu);
	wgpu->win = win;

	LOG_VERBOSE("Creating WebGPU instance");

	WGPUInstanceDescriptor const create_instance_descr = {};
	wgpu->instance = wgpuCreateInstance(&create_instance_descr);

	if (!wgpu->instance) {
		LOG_ERROR("Failed to create WebGPU instance");
		goto err;
	}

	LOG_VERBOSE("Creating surface");

	WGPUSurfaceDescriptorFromXcbWindow const surface_from_xcb_descr = {
		.chain = (WGPUChainedStruct const) {
			.sType = WGPUSType_SurfaceDescriptorFromXcbWindow,
		},
		.connection = win->connection,
		.window = win->win,
	};

	WGPUSurfaceDescriptor const surface_descr = {
		.nextInChain = (WGPUChainedStruct const*) &surface_from_xcb_descr ,
	};

	wgpu->surface = wgpuInstanceCreateSurface(wgpu->instance, &surface_descr);

	if (!wgpu->surface) {
		LOG_ERROR("Failed to create WebGPU surface from window");
		goto err;
	}

	LOG_VERBOSE("Requesting WebGPU adapter");

	WGPURequestAdapterOptions const request_adapter_options = { .compatibleSurface = wgpu->surface };
	wgpuInstanceRequestAdapter(wgpu->instance, &request_adapter_options, request_adapter_cb, wgpu);

	if (!wgpu->adapter) {
		LOG_ERROR("Failed to get WebGPU adapter");
		goto err;
	}

	WGPUAdapterProperties adapter_props;
	wgpuAdapterGetProperties(wgpu->adapter, &adapter_props);

	LOG_INFO("Got adapter: %s (%s)", adapter_props.name, adapter_props.driverDescription);
	LOG_VERBOSE("Requesting WebGPU device");

	wgpuAdapterRequestDevice(wgpu->adapter, NULL, request_device_cb, wgpu);

	if (!wgpu->device) {
		LOG_ERROR("Failed to get WebGPU device");
		goto err;
	}

	// TODO shader module

	return wgpu;

err:

	backend_wgpu_destroy(wgpu);
	return NULL;
}

void backend_wgpu_destroy(void* _wgpu) {
	backend_wgpu_t* const wgpu = _wgpu;

	if (wgpu->device) {
		wgpuDeviceRelease(wgpu->device);
	}

	if (wgpu->adapter) {
		wgpuAdapterRelease(wgpu->adapter);
	}

	if (wgpu->surface) {
		wgpuSurfaceRelease(wgpu->surface);
	}

	if (wgpu->instance) {
		wgpuInstanceRelease(wgpu->instance);
	}

	free(wgpu);
}
