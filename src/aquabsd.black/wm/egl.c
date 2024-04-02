// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2024 Aymeric Wibo

// a lot of the code here is lifted from wlroots/render/egl.c

#include "egl.h"

#include <assert.h>
#include <stdlib.h>

#include <xf86drm.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

static bool check_ext(char const* haystack, char const* needle) {
	size_t const needle_len = strlen(needle);
	char const* end = haystack + strlen(haystack);

	while (haystack < end) {
		if (*haystack == ' ') {
			haystack++;
			continue;
		}

		size_t const n = strcspn(haystack, " ");

		if (n == needle_len && strncmp(needle, haystack, n) == 0) {
			return true;
		}

		haystack += n;
	}

	return false;
}

char const* egl_error_str(void) {
	EGLint const error = eglGetError();

#define ERROR_CASE(error) \
	case error: return #error;

	switch (error) {
	ERROR_CASE(EGL_SUCCESS            )
	ERROR_CASE(EGL_NOT_INITIALIZED    )
	ERROR_CASE(EGL_BAD_ACCESS         )
	ERROR_CASE(EGL_BAD_ALLOC          )
	ERROR_CASE(EGL_BAD_ATTRIBUTE      )
	ERROR_CASE(EGL_BAD_CONTEXT        )
	ERROR_CASE(EGL_BAD_CONFIG         )
	ERROR_CASE(EGL_BAD_CURRENT_SURFACE)
	ERROR_CASE(EGL_BAD_DISPLAY        )
	ERROR_CASE(EGL_BAD_SURFACE        )
	ERROR_CASE(EGL_BAD_MATCH          )
	ERROR_CASE(EGL_BAD_PARAMETER      )
	ERROR_CASE(EGL_BAD_NATIVE_PIXMAP  )
	ERROR_CASE(EGL_BAD_NATIVE_WINDOW  )
	ERROR_CASE(EGL_CONTEXT_LOST       )

	default:
		return "unknown EGL error; consider setting 'EGL_LOG_LEVEL=debug'";
	}

	#undef ERROR_CASE
}

int egl_from_drm_fd(renderer_t* renderer) {
	wm_t* const wm = renderer->wm;
	int rv = -1;

	LOG_VERBOSE("Getting client extensions (EGL_EXT_client_extensions, EGL_BAD_DISPLAY means this extenion is not supported)");
	char const* const client_extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);

	if (client_extensions == NULL) {
		LOG_ERROR("Failed to get EGL client extensions: %s", egl_error_str());
		goto err_client_extensions;
	}

	LOG_INFO("EGL client extensions: %s", client_extensions);

	LOG_VERBOSE("Check for EGL_EXT_device_enumeration");
	// for eglQueryDevicesEXT

	if (!check_ext(client_extensions, "EGL_EXT_device_enumeration")) {
		LOG_ERROR("EGL_EXT_device_enumeration is not supported");
		goto err_missing_client_extension;
	}

	LOG_VERBOSE("Check for EGL_EXT_device_query");
	// for eglQueryDeviceStringEXT

	if (!check_ext(client_extensions, "EGL_EXT_device_query")) {
		LOG_ERROR("EGL_EXT_device_query is not supported");
		goto err_missing_client_extension;
	}

	LOG_VERBOSE("Check for EGL_EXT_platform_base");
	// for eglGetPlatformDisplayEXT

	if (!check_ext(client_extensions, "EGL_EXT_platform_base")) {
		LOG_ERROR("EGL_EXT_platform_base is not supported");
		goto err_missing_client_extension;
	}

	LOG_VERBOSE("Get list of EGLDeviceEXT's");

	PFNEGLQUERYDEVICESEXTPROC const eglQueryDevicesEXT = (void*) eglGetProcAddress("eglQueryDevicesEXT");
	assert(eglQueryDevicesEXT != NULL);

	EGLint egl_device_count = 0;

	if (!eglQueryDevicesEXT(0, NULL, &egl_device_count)) {
		LOG_ERROR("Failed to query device count: %s", egl_error_str());
		goto err_egl_query_devices_ext_count;
	}

	EGLDeviceEXT* const egl_devices = calloc(egl_device_count, sizeof *egl_devices);

	if (!eglQueryDevicesEXT(egl_device_count, egl_devices, &egl_device_count)) {
		LOG_ERROR("Failed to query devices: %s", egl_error_str());
		goto err_egl_query_devices_ext_for_realsies;
	}

	LOG_INFO("Found %d EGLDeviceEXT's", egl_device_count);

	LOG_VERBOSE("Get DRM device from fd (%d)", wm->drm_fd);

	drmDevice* drm_device = NULL;
	int const rv_drm_get_device = drmGetDevice(wm->drm_fd, &drm_device);

	if (rv_drm_get_device < 0) {
		LOG_ERROR("Failed to get DRM device: %s", strerror(-rv_drm_get_device));
		goto err_drm_get_device;
	}

	LOG_VERBOSE("Find EGLDeviceEXT matching one of the nodes on our DRM device");

	PFNEGLQUERYDEVICESTRINGEXTPROC const eglQueryDeviceStringEXT = (void*) eglGetProcAddress("eglQueryDeviceStringEXT");
	assert(eglQueryDeviceStringEXT != NULL);

	EGLDeviceEXT chosen_egl_device = EGL_NO_DEVICE_EXT;
	char const* egl_device_name = NULL;

	for (size_t i = 0; i < (size_t) egl_device_count; i++) {
		EGLDeviceEXT const egl_device = egl_devices[i];
		egl_device_name = eglQueryDeviceStringEXT(egl_device, EGL_DRM_DEVICE_FILE_EXT);

		if (egl_device_name == NULL) {
			LOG_WARN("Failed to get name of EGLDeviceEXT %zu", i);
			continue;
		}

		LOG_VERBOSE("Check EGLDeviceEXT %s", egl_device_name);

		for (size_t j = 0; j < DRM_NODE_MAX; j++) {
			if (!(drm_device->available_nodes & (1 << j))) {
				continue;
			}

			if (strcmp(drm_device->nodes[j], egl_device_name) == 0) {
				chosen_egl_device = egl_device;

				i = SIZE_MAX - 1;
				break;
			}
		}
	}

	if (chosen_egl_device == EGL_NO_DEVICE_EXT) {
		LOG_ERROR("Failed to find EGLDeviceEXT matching one of the nodes on our DRM device");
		goto err_egl_device_not_found;
	}

	LOG_INFO("Found EGLDeviceEXT matching one of the nodes on our DRM device: %s", egl_device_name);

	LOG_VERBOSE("Create EGLDisplay from EGLDeviceEXT");

	PFNEGLGETPLATFORMDISPLAYEXTPROC const eglGetPlatformDisplayEXT = (void*) eglGetProcAddress("eglGetPlatformDisplayEXT");
	assert(eglGetPlatformDisplayEXT != NULL);

	EGLDisplay const display = eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, chosen_egl_device, NULL);

	if (display == EGL_NO_DISPLAY) {
		LOG_ERROR("Failed to get EGL display: %s", egl_error_str());
		goto err_egl_get_platform_display;
	}

	LOG_VERBOSE("Initialize EGL display");
	EGLint major, minor;

	if (!eglInitialize(display, &major, &minor)) {
		LOG_ERROR("Failed to initialize EGL display: %s", egl_error_str());
		goto err_egl_initialize;
	}

	LOG_INFO("EGL version: %d.%d", major, minor);
	LOG_INFO("EGL vendor: %s", eglQueryString(display, EGL_VENDOR))

	LOG_VERBOSE("Getting display extensions");
	char const* const display_extensions = eglQueryString(display, EGL_EXTENSIONS);

	if (display_extensions == NULL) {
		LOG_ERROR("Failed to get EGL display extensions: %s", egl_error_str());
		goto err_display_extensions;
	}

	LOG_INFO("EGL display extensions: %s", display_extensions);

	LOG_VERBOSE("Check for EGL_KHR_no_config_context or EGL_MESA_configless_context");
	// for EGL_NO_CONFIG_KHR

	if (
		!check_ext(display_extensions, "EGL_KHR_no_config_context") &&
		!check_ext(display_extensions, "EGL_MESA_configless_context")
	) {
		LOG_ERROR("EGL_KHR_no_config_context or EGL_MESA_configless_context is not supported");
		goto err_missing_display_extension;
	}

	LOG_VERBOSE("Check for EGL_EXT_image_dma_buf_import");
	// for eglCreateImageKHR

	if (!check_ext(display_extensions, "EGL_EXT_image_dma_buf_import")) {
		LOG_ERROR("EGL_EXT_image_dma_buf_import is not supported");
		goto err_missing_display_extension;
	}

	// TODO here, in wlroots/render/egl.c:egl_init_display, the chosen device is checked for software rendering
	// this is done with eglQueryDeviceStringEXT(..., EGL_EXTENSIONS) and checking if EGL_MESA_device_software is a device extension
	// we should warn the user if this is the case

	LOG_VERBOSE("Create EGL context");
	EGLContext const context = eglCreateContext(display, EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT, NULL);

	// TODO request high priority context if EGL_IMG_context_priority is supported and we're running as the DRM master

	if (context == EGL_NO_CONTEXT) {
		LOG_ERROR("Failed to create EGL context: %s", egl_error_str());
		goto err_egl_create_context;
	}

	renderer->egl_device = chosen_egl_device;
	renderer->egl_display = display;
	renderer->egl_context = context;

	rv = 0;

err_egl_create_context:
err_missing_display_extension:
err_display_extensions:
err_egl_initialize:
err_egl_get_platform_display:
err_egl_device_not_found:

	drmFreeDevice(&drm_device);

err_drm_get_device:
err_egl_query_devices_ext_for_realsies:

	free(egl_devices);

err_egl_query_devices_ext_count:
err_missing_client_extension:
err_client_extensions:

	return rv;
}
