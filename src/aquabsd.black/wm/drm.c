// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2024 Aymeric Wibo

#include "drm.h"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <xf86drm.h>

int open_preferred_drm_fd(wm_t* wm, int* drm_fd_ref, bool* own_drm_fd_ref) {
	// similar to wlroots/render/wlr_renderer.c:open_preferred_drm_fd

	LOG_VERBOSE("Check if user has set AQUABSD_BLACK_WM_RENDER_DRM_DEVICE");
	char const* const drm_device = getenv("AQUABSD_BLACK_WM_RENDER_DRM_DEVICE");

	if (drm_device != NULL) {
		LOG_INFO("User set AQUABSD_BLACK_WM_RENDER_DRM_DEVICE to %s, attempt to open that DRM device", drm_device);
		int const drm_fd = open(drm_device, O_RDWR | O_CLOEXEC);

		if (drm_fd < 0) {
			LOG_ERROR("Failed to open DRM device: %s", drm_device, strerror(errno));
			return -1;
		}

		if (drmGetNodeTypeFromFd(drm_fd) != DRM_NODE_RENDER) {
			LOG_ERROR("Device %s is not a render node", drm_device);
			close(drm_fd);
			return -1;
		}

		*drm_fd_ref = drm_fd;
		*own_drm_fd_ref = true;

		return 0;
	}

	LOG_VERBOSE("Use DRM device from the wlroots backend");
	int const backend_drm_fd = wlr_backend_get_drm_fd(wm->backend);

	if (backend_drm_fd >= 0) {
		LOG_INFO("Using DRM device from the wlroots backend");

		*drm_fd_ref = backend_drm_fd;
		*own_drm_fd_ref = false;

		return 0;
	}

	LOG_ERROR("Could not find a DRM device to render with");
	LOG_FATAL("TODO we should still pick an arbitrary DRM device to render with (see wlroots/render/wlr_renderer.c:open_drm_render_node and drmGetDevices2 - actually, this should even be done at the top of the function so we can log the DRM devices available)");

	return -1;
}
