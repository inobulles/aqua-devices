// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2023 Aymeric Wibo

#include <font.h>
#include <ui.h>

int font_man_create(font_man_t* man) {
	memset(man, 0, sizeof *man);

	// query device and get relevant functions

	man->font_device = kos_query_device(0, (uint64_t) "aquabsd.alps.font");

	if (man->font_device == (uint64_t) -1) {
		LOG_FATAL("Failed to query 'aquabsd.alps.font' device");
		return -1;
	}

	aquabsd_alps_font_load_font = kos_load_device_function(man->font_device, "load_font");
	aquabsd_alps_font_free_font = kos_load_device_function(man->font_device, "free_font");

	// load fonts

	man->title = aquabsd_alps_font_load_font("sans bold 36");
	man->regular = aquabsd_alps_font_load_font("sans 12");

	if (man->title == NULL || man->regular == NULL) {
		font_man_destroy(man);
		return -1;
	}

	return 0;
}

void font_man_destroy(font_man_t* man) {
	if (man->title) {
		aquabsd_alps_font_free_font(man->title);
	}

	if (man->regular) {
		aquabsd_alps_font_free_font(man->regular);
	}
}

aquabsd_alps_font_text_t* font_man_gen(elem_t* elem, font_t* font, char const* __attribute__((unused)) str, colour_t __attribute__((unused)) colour) {
	ui_t* const ui = elem->ui;
	font_man_t* const man = &ui->font_man;

	if (font) {
		goto just_create_the_damn_text;
	}

	// get font depending on element type

	font_t* const LUT[] = {
		NULL, // ELEM_KIND_ROOT
		man->title, // ELEM_KIND_TITLE
		man->regular, // ELEM_KIND_PARAGRAPH
		man->regular, // ELEM_KIND_BUTTON
	};

	if (elem->kind >= sizeof LUT / sizeof *LUT) { // NOLINT
		LOG_ERROR("Please update the font LUT");
		return NULL;
	}

	font = LUT[elem->kind];

	if (!font) {
		LOG_ERROR("Element kind '%s' doesn't have an associated font", STR_ENUM(elem_kind_t_strs, elem->kind));
	}

just_create_the_damn_text:

	return NULL;
}
