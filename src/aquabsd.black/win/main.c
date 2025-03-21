// This Source Form is subject to the terms of the AQUA Software License, v. 1.0.
// Copyright (c) 2024 Aymeric Wibo

#include "win.h"

typedef enum {
	// window creation/destruction commands

	CMD_CREATE = 0x6377, // 'cw'
	CMD_DESTROY = 0x6477, // 'dw'

	// window event commands

	CMD_REGISTER_CB = 0x7263, // 'rc'
	CMD_LOOP = 0x6C6F, // 'lo'

	// window framebuffer commands

	CMD_GET_FB = 0x6662, // 'fb'

	// window attribute commands

	CMD_GET_DT_MS = 0x6474, // 'dt'

	CMD_GET_X_RES = 0x7872, // 'xr'
	CMD_GET_Y_RES = 0x7972, // 'yr'

	CMD_SET_CAPTION = 0x7363, // 'sc'
} cmd_t;

uint64_t (*kos_callback) (uint64_t callback, int argument_count, ...);

uint64_t send(uint16_t _cmd, void* data) {
	cmd_t const cmd = _cmd;
	uint64_t* const args = data;

	switch (cmd) {
	case CMD_CREATE:

		return (uint64_t) win_create(args[0], args[1], args[2]);

	case CMD_DESTROY:

		win_destroy((win_t*) args[0]);
		return 0;

	case CMD_REGISTER_CB:

		win_register_cb((win_t*) args[0], (win_cb_kind_t) args[1], args[2], args[3]);
		return 0;

	case CMD_LOOP:

		return (uint64_t) win_loop((win_t*) args[0]);

	case CMD_GET_FB:

		return (uint64_t) win_get_fb((win_t*) args[0]);

	case CMD_GET_DT_MS:

		return (uint64_t) win_get_dt_ms((win_t*) args[0]);

	case CMD_GET_X_RES:
		
		return (uint64_t) win_get_x_res((win_t*) args[0]);

	case CMD_GET_Y_RES:

		return (uint64_t) win_get_y_res((win_t*) args[0]);

	case CMD_SET_CAPTION:

		return (uint64_t) win_set_caption((win_t*) args[0], (char const*) args[1]);
	}

	return -1;
}
