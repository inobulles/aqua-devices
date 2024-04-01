// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2024 Aymeric Wibo

#include "wm.h"

typedef enum {
	// WM creation/destruction commands

	CMD_CREATE = 0x6377, // 'cw'
	CMD_DESTROY = 0x6477, // 'dw'

	// WM event commands

	CMD_REGISTER_CB = 0x7263, // 'rc'
	CMD_LOOP = 0x6C6F, // 'lo'
} cmd_t;

uint64_t (*kos_callback) (uint64_t callback, int argument_count, ...);

uint64_t send(uint16_t _cmd, void* data) {
	cmd_t const cmd = _cmd;
	uint64_t* const args = data;

	switch (cmd) {
	case CMD_CREATE:

		return (uint64_t) wm_create(args[0]);

	case CMD_DESTROY:

		wm_destroy((wm_t*) args[0]);
		return 0;

	case CMD_REGISTER_CB:
		
		wm_register_cb((wm_t*) args[0], (wm_cb_kind_t) args[1], args[2], args[3]);
		return 0;

	case CMD_LOOP:
		
		return (uint64_t) wm_loop((wm_t*) args[0]);
	}

	return -1;
}
