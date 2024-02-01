// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2023 Aymeric Wibo

void backend_dummy_destroy(void* internal) {
	(void) internal;
}

int backend_dummy_prerender(void* internal) {
	(void) internal;
	return 0;
}

int backend_dummy_postrender(void* internal) {
	(void) internal;
	return 0;
}
