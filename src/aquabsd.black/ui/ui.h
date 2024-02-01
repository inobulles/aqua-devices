// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2023 Aymeric Wibo

#pragma once

#include <common.h>
#include <font.h>
#include <cache.h>

typedef struct {
	bool valid;
	char const* name;
	void* internal;

	// operations

	void (*destroy) (void* internal);

	int (*prerender) (void* internal);
	int (*postrender) (void* internal);
} backend_t;

struct ui_t {
	backend_t backend;
	elem_t* root;

	font_man_t font_man;
	cache_t cache;
};

// different backends

#include <backends/dummy.h>

#if defined(WITH_WGPU)
#include <backends/wgpu.h>
#endif

ui_t* ui_create(char const* backend_name, uint64_t* args);
void ui_destroy(ui_t* ui);

int ui_render(ui_t* ui);
