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

static void texture_view_err_cb(WGPUErrorType type, char const* msg, void* data) {
	(void) data;

	if (type == WGPUErrorType_NoError) {
		return;
	}

	LOG_WARN("%s: type = %#.8x, message = \"%s\"", __func__, type, msg);
}

int backend_wgpu_prerender(void* _wgpu) {
	backend_wgpu_t* const wgpu = _wgpu;

	// get texture view for next frame

	wgpuDevicePushErrorScope(wgpu->device, WGPUErrorFilter_Validation);
	wgpu->texture_view = wgpuSwapChainGetCurrentTextureView(wgpu->swapchain);
	wgpuDevicePopErrorScope(wgpu->device, texture_view_err_cb, wgpu);

	if (!wgpu->texture_view) {
		LOG_ERROR("Failed to get current texture view");
		goto err_texture_view;
	}

	// create command encoder

	WGPUCommandEncoderDescriptor const command_encoder_descr = { .label = "command_encoder" };
	wgpu->command_encoder = wgpuDeviceCreateCommandEncoder(wgpu->device, &command_encoder_descr);

	if (!wgpu->command_encoder) {
		LOG_ERROR("Failed to create command encoder");
		goto err_command_encoder;
	}

	// create render pass encoder

	WGPURenderPassColorAttachment const colour_attachments[] = {
		{
			.view = wgpu->texture_view,
			.loadOp = WGPULoadOp_Clear,
			.storeOp = WGPUStoreOp_Store,
			.clearValue = (WGPUColor const) { 0, 0, 0, 0 },
		},
	};

	WGPURenderPassDescriptor const render_pass_descr = {
		.label = "render_pass_encoder",
		.colorAttachmentCount = sizeof colour_attachments / sizeof *colour_attachments,
		.colorAttachments = colour_attachments,
	};

	wgpu->render_pass_encoder = wgpuCommandEncoderBeginRenderPass(wgpu->command_encoder, &render_pass_descr);

	if (!wgpu->render_pass_encoder) {
		LOG_ERROR("Failed to create render pass encoder");
		goto err_render_pass_encoder;
	}

	wgpuRenderPassEncoderSetPipeline(wgpu->render_pass_encoder, wgpu->render_pipeline);
	wgpuRenderPassEncoderDraw(wgpu->render_pass_encoder, 3, 1, 0, 0);
	wgpuRenderPassEncoderEnd(wgpu->render_pass_encoder);

	// create final command buffer

	WGPUCommandBufferDescriptor const command_buffer_descr = { .label = "command_buffer" };
	wgpu->command_buffer = wgpuCommandEncoderFinish(wgpu->command_encoder, &command_buffer_descr);

	if (!wgpu->command_buffer) {
		LOG_ERROR("Failed to create command buffer");
		goto err_command_buffer;
	}

	return 0;

err_command_buffer:

	wgpuRenderPassEncoderRelease(wgpu->render_pass_encoder);

err_render_pass_encoder:

	wgpuCommandEncoderRelease(wgpu->command_encoder);

err_command_encoder:

	wgpuTextureViewRelease(wgpu->texture_view);

err_texture_view:

	return -1;
}

int backend_wgpu_postrender(void* _wgpu) {
	backend_wgpu_t* const wgpu = _wgpu;

	// submit command buffer to queue

	WGPUCommandBuffer const command_buffers[] = { wgpu->command_buffer };
	wgpuQueueSubmit(wgpu->queue, sizeof command_buffers / sizeof *command_buffers, command_buffers); // NOLINT

	// present swapchain

	wgpuSwapChainPresent(wgpu->swapchain);

	// cleanup

	wgpuCommandBufferRelease(wgpu->command_buffer);
	wgpuRenderPassEncoderRelease(wgpu->render_pass_encoder);
	wgpuCommandEncoderRelease(wgpu->command_encoder);
	wgpuTextureViewRelease(wgpu->texture_view);

	return 0;
}
