// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2023 Aymeric Wibo

#pragma once

#include <common.h>

#include <hashmap/hashmap.h>

typedef struct {
	struct hashmap* map;
} cache_t;

int cache_create(cache_t* cache);
void cache_destroy(cache_t* cache);
