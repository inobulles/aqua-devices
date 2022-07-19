// TODO support for multiple mice and counting/listing functions
//      maybe also network mice?

#include <aquabsd.alps.mouse/private.h>
#include <aquabsd.alps.mouse/functions.h>

typedef enum {
	CMD_GET_DEFAULT_MOUSE_ID = 0x646D, // 'dm'
	CMD_UPDATE_MOUSE         = 0x756D, // 'um'
	CMD_POLL_BUTTON          = 0x7062, // 'pb'
	CMD_POLL_AXIS            = 0x7061, // 'pa'
} cmd_t;

int load(void) {
	mouse_count = 0;
	mice = NULL;

#if defined(AQUABSD_CONSOLE_MOUSE)
	aquabsd_console_mouse_id = register_mouse("aquaBSD console mouse", update_aquabsd_console_mouse, NULL, 1)->id;
#endif

	return 0;
}

void quit(void) {
	free(mice);
}

uint64_t send(cmd_t _cmd, void* data) {
	cmd_t cmd = _cmd;
	uint64_t* args = data;

	if (cmd == CMD_GET_DEFAULT_MOUSE_ID) {
		return get_default_mouse_id();
	}

	else if (cmd == CMD_UPDATE_MOUSE) {
		unsigned mouse_id = args[0];
		return (uint64_t) update_mouse(mouse_id);
	}

	else if (cmd == CMD_POLL_BUTTON) {
		unsigned mouse_id = args[0];
		button_t button = args[1];

		return (uint64_t) poll_button(mouse_id, button);
	}

	else if (cmd == CMD_POLL_AXIS) {
		unsigned mouse_id = args[0];
		axis_t axis = args[1];

		float result = poll_axis(mouse_id, axis);
		return *(uint64_t*) &result;
	}

	return -1;
}
