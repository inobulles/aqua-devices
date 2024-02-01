// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2023 Aymeric Wibo

#include <elem.h>

#include <string.h>
#include <stdlib.h>

elem_t* elem_create(elem_t* parent, elem_kind_t kind) {
	elem_t* const elem = calloc(1, sizeof *elem);

	elem->ui = parent->ui;
	elem->kind = kind;

	if (kind != ELEM_ROOT) {
		elem->parent = parent;
		// TODO add to parent
	}
	
	return elem;
}

void elem_destroy(elem_t* elem) {
	elem_unset(elem);
	free(elem);
}

int elem_set_text(elem_t* elem, char const* text) {
	if (!ELEM_IS_TEXT(elem)) {
		return -1;
	}

	elem_unset(elem);
	elem->detail.title.text = strdup(text);

	return 0;
}

int elem_unset(elem_t* elem) {
	if (ELEM_IS_TEXT(elem)) {
		free(elem->detail.title.text);
	}

	return 0;
}
