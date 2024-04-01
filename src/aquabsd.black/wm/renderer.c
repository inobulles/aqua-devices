// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2024 Aymeric Wibo

#include "renderer.h"

#include <assert.h>
#include <stdlib.h>

#include <wlr/render/interface.h>
#include <wlr/render/wlr_renderer.h>

#include "egl.h"

#include <libdrm/drm_fourcc.h>

// TODO error handling

static renderer_t* get_renderer(struct wlr_renderer* wlr_renderer) {
	renderer_t* const renderer = wl_container_of(wlr_renderer, renderer, wlr_renderer);
	return renderer;
}

static int get_drm_fd(struct wlr_renderer* wlr_renderer) {
	renderer_t* const renderer = get_renderer(wlr_renderer);
	return renderer->wm->drm_fd;
}

static uint32_t get_render_buffer_caps(struct wlr_renderer* wlr_renderer) {
	(void) wlr_renderer;
	return WLR_BUFFER_CAP_DMABUF;
}

static struct wlr_drm_format_set const* get_render_formats(struct wlr_renderer* wlr_renderer) {
	renderer_t* const renderer = get_renderer(wlr_renderer);
	return &renderer->dmabuf_render_formats;
}

typedef struct {
	struct wlr_render_pass base;
	renderer_t* renderer;
	struct wlr_buffer* wlr_buffer;
} renderpass_t;

static bool renderpass_submit(struct wlr_render_pass* wlr_pass) {
	renderpass_t* const renderpass = wl_container_of(wlr_pass, renderpass, base);

	wlr_buffer_unlock(renderpass->wlr_buffer);
	free(renderpass);

	return true;
}

static void renderpass_add_rect(struct wlr_render_pass* wlr_pass, struct wlr_render_rect_options const* options) {
	(void) wlr_pass;
	(void) options;
}

static void renderpass_add_texture(struct wlr_render_pass* wlr_pass, struct wlr_render_texture_options const* options) {
	(void) wlr_pass;
	(void) options;
}

static struct wlr_render_pass_impl const renderpass_impl = {
	.submit = renderpass_submit,
	.add_rect = renderpass_add_rect,
	.add_texture = renderpass_add_texture,
};

static struct wlr_render_pass* begin_buffer_pass(struct wlr_renderer* wlr_renderer, struct wlr_buffer* wlr_buffer, struct wlr_buffer_pass_options const* options) {
	renderer_t* const renderer = get_renderer(wlr_renderer);
	wm_t* const wm = renderer->wm;

	(void) options;

	wm->x_res = wlr_buffer->width;
	wm->y_res = wlr_buffer->height;

	// XXX here, GLES2 does gles2_buffer_get_or_create
	// this does some stuff with EGL which should probably be in wgpu when swapping buffers or something

	struct wlr_dmabuf_attributes dmabuf = {0};

	if (!wlr_buffer_get_dmabuf(wlr_buffer, &dmabuf)) {
		goto error_buffer;
	}

	EGLDisplay const display = eglGetCurrentDisplay();

	size_t attrib_size = 8;
	EGLint attribs[64] = { // XXX arbitrary size which hopefully is enough :)
		EGL_WIDTH, dmabuf.width,
		EGL_HEIGHT, dmabuf.height,
		EGL_LINUX_DRM_FOURCC_EXT, dmabuf.format,
		EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,
	};

	EGLint const fd_delta = EGL_DMA_BUF_PLANE1_FD_EXT - EGL_DMA_BUF_PLANE0_FD_EXT;
	EGLint const offset_delta = EGL_DMA_BUF_PLANE1_OFFSET_EXT - EGL_DMA_BUF_PLANE0_OFFSET_EXT;
	EGLint const pitch_delta = EGL_DMA_BUF_PLANE1_PITCH_EXT - EGL_DMA_BUF_PLANE0_PITCH_EXT;
	EGLint const mod_lo_delta = EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT - EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT;
	EGLint const mod_hi_delta = EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT - EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT;

	for (size_t i = 0; i < (size_t) dmabuf.n_planes; i++) {
		attribs[attrib_size++] = EGL_DMA_BUF_PLANE0_FD_EXT + i * fd_delta;
		attribs[attrib_size++] = dmabuf.fd[i];

		attribs[attrib_size++] = EGL_DMA_BUF_PLANE0_OFFSET_EXT + i * offset_delta;
		attribs[attrib_size++] = dmabuf.offset[i];

		attribs[attrib_size++] = EGL_DMA_BUF_PLANE0_PITCH_EXT + i * pitch_delta;
		attribs[attrib_size++] = dmabuf.stride[i];

		// XXX egl->has_modifiers
		if (true || dmabuf.modifier == DRM_FORMAT_MOD_INVALID) {
			continue;
		}

		attribs[attrib_size++] = EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT + i * mod_lo_delta;
		attribs[attrib_size++] = dmabuf.modifier & 0xFFFFFFFF;

		attribs[attrib_size++] = EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT + i * mod_hi_delta;
		attribs[attrib_size++] = dmabuf.modifier >> 32;
	}

	attribs[attrib_size] = EGL_NONE;
	assert(attrib_size < sizeof attribs / sizeof *attribs);

	PFNEGLCREATEIMAGEKHRPROC const eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC) eglGetProcAddress("eglCreateImageKHR");
	EGLImage const image = eglCreateImageKHR(display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, attribs);

	if (image == EGL_NO_IMAGE_KHR) {
		LOG_FATAL("Oh no!");
		goto error_buffer;
	}

error_buffer: {}

	// XXX here, GLES2 does begin_gles2_buffer_pass

	renderpass_t* const renderpass = calloc(1, sizeof *renderpass);

	renderpass->renderer = renderer;
	renderpass->wlr_buffer = wlr_buffer;

	wlr_render_pass_init(&renderpass->base, &renderpass_impl);
	wlr_buffer_lock(wlr_buffer);

	return NULL;
}

static struct wlr_renderer_impl const renderer_impl = {
	.get_drm_fd = get_drm_fd,
	.get_render_buffer_caps = get_render_buffer_caps,
	.get_render_formats = get_render_formats,
	.begin_buffer_pass = begin_buffer_pass,
};

struct wlr_renderer* renderer_create(wm_t* wm) {
	// TODO in the future this should support Vulkan too (and especially)!
	// because right now it's capping WebGPU to just OpenGL adapters

	renderer_t* const renderer = calloc(1, sizeof *renderer);

	renderer->wm = wm;
	renderer->wlr_renderer.impl = &renderer_impl;

	LOG_VERBOSE("Creating EGL display from DRM fd (%d)", wm->drm_fd);

	if (egl_from_drm_fd(renderer) < 0) {
		free(renderer);
		return NULL;
	}

	// get render formats
	// equivalent to wlroots/src/render/egl.c:init_dmabuf_formats
	// TODO currently just hardcoded

	LOG_VERBOSE("Adding XR24 format to render formats (TODO)");

	uint32_t const XR24 = 0x34325258;
	uint64_t const modifiers = 0x020000001046BB04;

	wlr_drm_format_set_add(&renderer->dmabuf_render_formats, XR24, modifiers);

	LOG_VERBOSE("Making EGL context current");

	if (!eglMakeCurrent(renderer->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, renderer->egl_context)) {
		LOG_ERROR("Failed to make EGL context current: %s", egl_error_str());
		free(renderer);
		return NULL;
	}

	return &renderer->wlr_renderer;
}
