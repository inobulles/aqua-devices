// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2023 Aymeric Wibo

#pragma once

#include <aquabsd.alps/font/public.h>

#include <misc.h>
#include <elem.h>

typedef aquabsd_alps_font_t font_t;
typedef aquabsd_alps_font_text_t font_text_t;

typedef struct {
	uint64_t font_device;

	aquabsd_alps_font_t* title;
	aquabsd_alps_font_t* regular;
} font_man_t;

int font_man_create(font_man_t* man);
void font_man_destroy(font_man_t* man);
font_text_t* font_man_gen(elem_t* elem, font_t* font, char const* str, colour_t colour);
