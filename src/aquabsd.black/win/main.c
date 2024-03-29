// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2024 Aymeric Wibo

#include "win.h"

typedef enum {
	// window creation/deletion commands

	CMD_CREATE = 0x6377, // 'cw'
	CMD_DESTROY = 0x6477, // 'dw'
} cmd_t;

uint64_t send(uint16_t _cmd, void* data) {
	cmd_t const cmd = _cmd;
	uint64_t* const args = data;

	switch (cmd) {
	case CMD_CREATE:

		return (uint64_t) win_create(args[0], args[1]);

	case CMD_DESTROY:

		win_destroy((win_t*) args[0]);
		return 0;
	}

	return -1;
}
