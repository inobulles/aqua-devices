// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2024 Aymeric Wibo

#pragma once

#include "renderer.h"

char const* egl_error_str(void);
int egl_from_drm_fd(renderer_t* renderer);
