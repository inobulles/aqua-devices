// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2023 Aymeric Wibo

#pragma once

#include <common.h>

#include <stdlib.h>

#define elem_kind_t_ENUM(_) \
	_(ELEM_ROOT) \
	_(ELEM_TITLE) \
	_(ELEM_PARA) \
	_(ELEM_BUTTON) \

ENUM(elem_kind_t);

#define ELEM_IS_TEXT(elem) ( \
	(elem)->kind == ELEM_TITLE || \
	(elem)->kind == ELEM_PARA || \
	(elem)->kind == ELEM_BUTTON || \
0)

typedef struct {
	char* text;
} elem_text_t;

struct elem_t {
	ui_t* ui;
	elem_kind_t kind;
	elem_t* parent;

	size_t child_count;
	elem_t** children;

	union {
		elem_text_t title;
	} detail;
};
 
elem_t* elem_create(elem_t* parent, elem_kind_t kind);
void elem_destroy(elem_t* elem);

int elem_set_text(elem_t* elem, char const* text);
int elem_unset(elem_t* elem);
