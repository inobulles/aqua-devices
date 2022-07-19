// for now, you can only poll individual keyboard buttons, such as arrows
// in the future though, I'll have an event system to receive and unicode character, as I don't want applications to have to deal with keymaps

#include <aquabsd.alps.kbd/private.h>
#include <aquabsd.alps.kbd/functions.h>

typedef enum {
	CMD_GET_DEFAULT_KBD_ID = 0x646B, // 'dk'
	CMD_UPDATE_KBD         = 0x756B, // 'uk'
	CMD_POLL_BUTTON        = 0x7062, // 'pb'
	CMD_GET_BUF_LEN        = 0x626C, // 'bl'
	CMD_READ_BUF           = 0x7262, // 'rb'
} cmd_t;

int load(void) {
	kbd_count = 0;
	kbds = NULL;

#if defined(AQUABSD_CONSOLE_KBD)
	aquabsd_console_kbd_id = register_kbd("aquaBSD console keyboard", update_aquabsd_console_kbd, NULL, 1)->id;
#endif

	return 0;
}

void quit(void) {
	free(kbds);
}

uint64_t send(uint16_t _cmd, void* data) {
	cmd_t cmd = _cmd;
	uint64_t* args = data;

	if (cmd == CMD_GET_DEFAULT_KBD_ID) {
		return get_default_kbd_id();
	}

	else if (cmd == CMD_UPDATE_KBD) {
		unsigned kbd_id = args[0];
		return (uint64_t) update_kbd(kbd_id);
	}

	else if (cmd == CMD_POLL_BUTTON) {
		unsigned kbd_id = args[0];
		button_t button = args[1];

		return (uint64_t) poll_button(kbd_id, button);
	}

	else if (cmd == CMD_GET_BUF_LEN) {
		unsigned kbd_id = args[0];
		return get_buf_len(kbd_id);
	}

	else if (cmd == CMD_READ_BUF) {
		unsigned kbd_id = args[0];
		void* buf = (void*) args[1];

		return read_buf(kbd_id, buf);
	}

	return -1;
}
