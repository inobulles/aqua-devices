// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2024 Aymeric Wibo

#pragma once

#include "wm.h"

int open_preferred_drm_fd(wm_t* wm, int* drm_fd_ref, bool* own_drm_fd_ref);
