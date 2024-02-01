// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2023 Aymeric Wibo

#include <cache.h>

int cache_create(cache_t* cache) {
	(void) cache; // cache->map = hashmap_new();
	return 0;
}

void cache_destroy(cache_t* cache) {
	if (cache->map) {
		hashmap_free(cache->map);
	}
}
