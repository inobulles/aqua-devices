// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2023 Aymeric Wibo

#include <ui.h>
#include <elem.h>

typedef enum {
	// context creation/deletion commands

	CMD_CREATE      = 0x6363, // 'cc'
	CMD_DESTROY     = 0x6463, // 'dc'

	// context commands

	CMD_GET_ROOT    = 0x6772, // 'gr'
	CMD_RENDER      = 0x7263, // 'rc'

	// element commands

	CMD_ADD_ELEMENT = 0x6165, // 'ae'
	CMD_REM_ELEMENT = 0x7265, // 're'
	CMD_CLK_ELEMENT = 0x6365, // 'ce'
} cmd_t;

uint64_t (*kos_query_device) (uint64_t, uint64_t name);
void* (*kos_load_device_function) (uint64_t device, const char* name);
uint64_t (*kos_callback) (uint64_t callback, int argument_count, ...);

uint64_t fs_device = -1;

int load(void) {
	fs_device = kos_query_device(0, (uint64_t) "core.fs");

	if (fs_device == (uint64_t) -1) {
		LOG_FATAL("Failed to query 'core.fs' device");
		return -1;
	}

	return 0;
}

static int elem_set(elem_t* elem, uint64_t* args) {
	if (ELEM_IS_TEXT(elem)) {
		char const* const text = (void*) args[0];
		elem_set_text(elem, text);
	}

	return 0;
}

uint64_t send(uint16_t _cmd, void* data) {
	cmd_t const cmd = _cmd;
	uint64_t* const args = data;

	if (cmd == CMD_CREATE) {
		char* const backend_name = (void*) args[0];
		return (uint64_t) ui_create(backend_name, args + 1);
	}

	else if (cmd == CMD_DESTROY) {
		ui_t* const ui = (void*) args[0];

		ui_destroy(ui);
		return 0;
	}

	else if (cmd == CMD_GET_ROOT) {
		ui_t* const ui = (void*) args[0];
		return (uint64_t) ui->root;
	}

	else if (cmd == CMD_RENDER) {
		ui_t* const ui = (void*) args[0];
		return ui_render(ui);
	}

	else if (cmd == CMD_ADD_ELEMENT) {
		elem_t* const parent = (void*) args[0];
		elem_kind_t const kind = args[1];

		elem_t* const elem = elem_create(parent, kind);
		elem_set(elem, args + 2);

		return (uint64_t) elem;
	}

	else if (cmd == CMD_REM_ELEMENT) {
		LOG_ERROR("TODO");
	}

	else if (cmd == CMD_CLK_ELEMENT) {
		LOG_ERROR("TODO");
	}

	return -1;
}
