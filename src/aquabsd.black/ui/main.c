// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2023 Aymeric Wibo

#include <private.h>

typedef enum {
	// context creation/deletion commands

	CMD_CREATE  = 0x6363, // 'cc'
	CMD_DESTROY = 0x6463, // 'dc'
} cmd_t;

uint64_t (*kos_query_device) (uint64_t, uint64_t name);
void* (*kos_load_device_function) (uint64_t device, const char* name);
uint64_t (*kos_callback) (uint64_t callback, int argument_count, ...);

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

	return -1;
}
