// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2024 Aymeric Wibo

#include "renderer.h"

#include <assert.h>
#include <stdlib.h>

#include <wlr/render/interface.h>
#include <wlr/render/wlr_renderer.h>

#include "egl.h"

#include <libdrm/drm_fourcc.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

// TODO error handling

static int get_drm_fd(struct wlr_renderer* wlr_renderer) {
	renderer_t* const renderer = wm_renderer_container(wlr_renderer);
	return renderer->wm->drm_fd;
}

static uint32_t get_render_buffer_caps(struct wlr_renderer* wlr_renderer) {
	(void) wlr_renderer;
	return WLR_BUFFER_CAP_DMABUF;
}

static struct wlr_drm_format_set const* get_render_formats(struct wlr_renderer* wlr_renderer) {
	renderer_t* const renderer = wm_renderer_container(wlr_renderer);
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
	renderer_t* const renderer = wm_renderer_container(wlr_renderer);
	wm_t* const wm = renderer->wm;

	(void) options;

	wm->x_res = wlr_buffer->width;
	wm->y_res = wlr_buffer->height;

	// XXX here, GLES2 does gles2_buffer_get_or_create
	// this does some stuff with EGL which should probably be in wgpu when swapping buffers or something

	eglMakeCurrent(renderer->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, renderer->egl_context);

	// TODO fix up all this error handling!

	struct wlr_dmabuf_attributes dmabuf = {0};

	if (!wlr_buffer_get_dmabuf(wlr_buffer, &dmabuf)) {
		LOG_FATAL("Oh no! Couldn't get DMA-BUF");
	}

	bool const modifiers =
		dmabuf.modifier != DRM_FORMAT_MOD_INVALID &&
		dmabuf.modifier != DRM_FORMAT_MOD_LINEAR;

	assert(modifiers); // TODO what to do when no modifiers?

	LOG_INFO("Got %dx%d DMA-BUF buffer with %d planes (with%s modifiers)", dmabuf.width, dmabuf.height, dmabuf.n_planes, modifiers ? "" : "out");

	EGLint attribs[] = {
		EGL_WIDTH, dmabuf.width,
		EGL_HEIGHT, dmabuf.height,
		EGL_LINUX_DRM_FOURCC_EXT, dmabuf.format,
		EGL_IMAGE_PRESERVED_KHR, EGL_TRUE,

	dmabuf.n_planes < 1 ? EGL_NONE :
		EGL_DMA_BUF_PLANE0_FD_EXT, dmabuf.fd[0],
		EGL_DMA_BUF_PLANE0_OFFSET_EXT, dmabuf.offset[0],
		EGL_DMA_BUF_PLANE0_PITCH_EXT, dmabuf.stride[0],
		EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT, dmabuf.modifier & 0xFFFFFFFF,
		EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, dmabuf.modifier >> 32,

	dmabuf.n_planes < 2 ? EGL_NONE :
		EGL_DMA_BUF_PLANE1_FD_EXT, dmabuf.fd[1],
		EGL_DMA_BUF_PLANE1_OFFSET_EXT, dmabuf.offset[1],
		EGL_DMA_BUF_PLANE1_PITCH_EXT, dmabuf.stride[1],
		EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT, dmabuf.modifier & 0xFFFFFFFF,
		EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT, dmabuf.modifier >> 32,

	dmabuf.n_planes < 3 ? EGL_NONE :
		EGL_DMA_BUF_PLANE2_FD_EXT, dmabuf.fd[2],
		EGL_DMA_BUF_PLANE2_OFFSET_EXT, dmabuf.offset[2],
		EGL_DMA_BUF_PLANE2_PITCH_EXT, dmabuf.stride[2],
		EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT, dmabuf.modifier & 0xFFFFFFFF,
		EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT, dmabuf.modifier >> 32,

	dmabuf.n_planes < 4 ? EGL_NONE :
		EGL_DMA_BUF_PLANE3_FD_EXT, dmabuf.fd[3],
		EGL_DMA_BUF_PLANE3_OFFSET_EXT, dmabuf.offset[3],
		EGL_DMA_BUF_PLANE3_PITCH_EXT, dmabuf.stride[3],
		EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT, dmabuf.modifier & 0xFFFFFFFF,
		EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT, dmabuf.modifier >> 32,

		EGL_NONE
	};

	PFNEGLCREATEIMAGEKHRPROC const eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC) eglGetProcAddress("eglCreateImageKHR");
	EGLImage const image = eglCreateImageKHR(renderer->egl_display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, attribs);

	if (image == EGL_NO_IMAGE_KHR) {
		LOG_FATAL("Oh no! %s", egl_error_str());
		goto error_buffer;
	}

error_buffer: {}

	// XXX here, GLES2 does begin_gles2_buffer_pass
	// this is super temporary stuff just for testing

	GLuint rbo, fbo;

	PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC const glEGLImageTargetRenderbufferStorageOES = (void*) eglGetProcAddress("glEGLImageTargetRenderbufferStorageOES");

	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glEGLImageTargetRenderbufferStorageOES(GL_RENDERBUFFER, image);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		LOG_FATAL("Oh no! Framebuffer is not complete");
	}

	LOG_FATAL("Framebuffer is complete");

	glClearColor(1.0, 0.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glFlush();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	renderpass_t* const renderpass = calloc(1, sizeof *renderpass);

	renderpass->renderer = renderer;
	renderpass->wlr_buffer = wlr_buffer;

	wlr_render_pass_init(&renderpass->base, &renderpass_impl);
	wlr_buffer_lock(wlr_buffer);
	wlr_buffer_unlock(wlr_buffer);

	return &renderpass->base;
}

static struct wlr_renderer_impl const renderer_impl = {
	.get_drm_fd = get_drm_fd,
	.get_render_buffer_caps = get_render_buffer_caps,
	.get_render_formats = get_render_formats,
	.begin_buffer_pass = begin_buffer_pass,
};

struct wlr_renderer* renderer_create(wm_t* wm) {
	// TODO in the future this should support Vulkan too (and especially!)
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
	// also we just assume support for EGL DMA-BUF format modifiers - check this (see egl->has_modifiers)

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

	LOG_VERBOSE("OpenGL should be available by now, listing information");

	LOG_INFO("OpenGL version: %s", glGetString(GL_VERSION));
	LOG_INFO("OpenGL vendor: %s", glGetString(GL_VENDOR));
	LOG_INFO("OpenGL renderer: %s", glGetString(GL_RENDERER));
	LOG_INFO("OpenGL extensions: %s", glGetString(GL_EXTENSIONS));

	return &renderer->wlr_renderer;
}
