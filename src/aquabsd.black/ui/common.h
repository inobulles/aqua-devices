// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2023 Aymeric Wibo

#pragma once

#include <stdbool.h>

#include <umber.h>
#define UMBER_COMPONENT "aquabsd.black.ui"

// KOS-related stuff

extern uint64_t (*kos_query_device) (uint64_t, uint64_t name);
extern void* (*kos_load_device_function) (uint64_t device, const char* name);
extern uint64_t (*kos_callback) (uint64_t callback, int argument_count, ...);

// helpful macros for later debugging

#define GEN_ENUM(NAME) NAME,
#define GEN_STRS(STR) #STR,

#define ENUM(T) \
	typedef enum { T##_ENUM(GEN_ENUM) } T; \
	__attribute__((unused)) static char const* const T##_strs[] = { T##_ENUM(GEN_STRS) }

#define STR_ENUM(strs, i) ((int64_t) (i) < 0 || (size_t) (i) >= sizeof((strs)) / sizeof(*(strs)) ? "<unknown>" : (strs)[(int64_t) (i)])

// forward declarations

typedef struct ui_t ui_t;
typedef struct elem_t elem_t;
