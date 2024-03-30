// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2024 Aymeric Wibo

#pragma once

#include <stdint.h>

#include <umber.h>
#define UMBER_COMPONENT "aquabsd.black.win"

// KOS-related stuff

extern uint64_t (*kos_query_device) (uint64_t, uint64_t name);
extern void* (*kos_load_device_function) (uint64_t device, const char* name);
extern uint64_t (*kos_callback) (uint64_t callback, int argument_count, ...);
