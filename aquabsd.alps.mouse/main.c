// TODO support for multiple mice and counting/listing functions
//      maybe also network mice?

#include <aquabsd.alps.mouse/private.h>
#include <aquabsd.alps.mouse/functions.h>

#define CMD_GET_DEFAULT_MOUSE_ID 0x646D // 'dm'
#define CMD_UPDATE_MOUSE         0x756D // 'um'
#define CMD_POLL_BUTTON          0x7062 // 'pb'
#define CMD_POLL_AXIS            0x7061 // 'pa'

int load(
	uint64_t (*kos_query_device) (uint64_t, uint64_t name),
	void* (*kos_load_device_function) (uint64_t device, const char* name),
	uint64_t (*kos_callback) (uint64_t callback, int argument_count, ...)) {

	mouse_count = 0;
	mice = NULL;

	#if defined(AQUABSD_CONSOLE_MOUSE)
		aquabsd_console_mouse_id = register_mouse("aquaBSD console mouse", update_aquabsd_console_mouse, 1)->id;
	#endif

	return 0;
}

void quit(void) {
	free(mice);
}

uint64_t send(uint16_t command, void* data) {
	uint64_t* arguments = (uint64_t*) data;

	if (command == CMD_GET_DEFAULT_MOUSE_ID) {
		return get_default_mouse_id();
	}

	else if (command == CMD_UPDATE_MOUSE) {
		unsigned mouse_id = arguments[0];
		return (uint64_t) update_mouse(mouse_id);
	}

	else if (command == CMD_POLL_BUTTON) {
		unsigned mouse_id = arguments[0];
		button_t button = arguments[1];

		return (uint64_t) poll_button(mouse_id, button);
	}

	else if (command == CMD_POLL_AXIS) {
		unsigned mouse_id = arguments[0];
		axis_t axis = arguments[1];

		float result = poll_axis(mouse_id, axis);
		return *(uint64_t*) &result;
	}

	return -1;
}