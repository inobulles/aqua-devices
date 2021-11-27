// for now, you can only poll individual keyboard buttons, such as arrows
// in the future though, I'll have an event system to receive and unicode character, as I don't want applications to have to deal with keymaps

#include <aquabsd.alps.kbd/private.h>
#include <aquabsd.alps.kbd/functions.h>

#define CMD_GET_DEFAULT_KBD_ID 0x646B // 'dk'
#define CMD_UPDATE_KBD         0x756B // 'uk'
#define CMD_POLL_BUTTON        0x7062 // 'pb'
#define CMD_GET_BUF_LEN        0x626C // 'bl'
#define CMD_READ_BUF           0x7262 // 'rb'

int load(
	uint64_t (*kos_query_device) (uint64_t, uint64_t name),
	void* (*kos_load_device_function) (uint64_t device, const char* name),
	uint64_t (*kos_callback) (uint64_t callback, int argument_count, ...)) {

	kbd_count = 0;
	kbds = NULL;

	#if defined(AQUABSD_CONSOLE_KBD)
		if (init_aquabsd_console_kbd() == 0) {
			aquabsd_console_kbd_id = register_kbd("aquaBSD console keyboard", update_aquabsd_console_kbd, NULL, 1)->id;
		}
	#endif

	return 0;
}

void quit(void) {
	free(kbds);
}

uint64_t send(uint16_t command, void* data) {
	uint64_t* arguments = (uint64_t*) data;

	if (command == CMD_GET_DEFAULT_KBD_ID) {
		return get_default_kbd_id();
	}

	else if (command == CMD_UPDATE_KBD) {
		unsigned kbd_id = arguments[0];
		return (uint64_t) update_kbd(kbd_id);
	}

	else if (command == CMD_POLL_BUTTON) {
		unsigned kbd_id = arguments[0];
		button_t button = arguments[1];

		return (uint64_t) poll_button(kbd_id, button);
	}

	else if (command == CMD_GET_BUF_LEN) {
		unsigned kbd_id = arguments[0];
		return get_buf_len(kbd_id);
	}

	else if (command == CMD_READ_BUF) {
		unsigned kbd_id = arguments[0];
		void* buf = (void*) arguments[1];

		return read_buf(kbd_id, buf);
	}

	return -1;
}
