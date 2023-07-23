// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2023 Aymeric Wibo

#pragma once

#include <stdbool.h>

#include <umber.h>
#define UMBER_COMPONENT "aquabsd.black.ui"

typedef struct {
	bool valid;
	char const* name;
	void* internal;

	// operations

	void (*destroy) (void* internal);
} backend_t;

typedef struct {
	backend_t backend;
} ui_t;

// context functions

ui_t* ui_create(char* const backend_name, uint64_t* args);
void ui_destroy(ui_t* ui);

// different backends

#if defined(WITH_WGPU)
#include <backends/wgpu.h>
#endif
