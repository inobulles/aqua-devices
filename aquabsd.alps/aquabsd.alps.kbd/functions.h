#include <stdlib.h>
#include <string.h>

// functions accessible to AQUA programs and other devices

dynamic unsigned get_default_kbd_id(void) {
	return default_kbd_id;
}

dynamic int update_kbd(unsigned kbd_id) {
	kbd_t* kbd = &kbds[kbd_id];

	if (kbd->buf) {
		free(kbd->buf);
	}

	if (!kbd->update_callback) {
		return 0;
	}

	return kbd->update_callback(kbd, kbd->update_cb_param);
}

dynamic unsigned poll_button(unsigned kbd_id, button_t button) {
	return kbds[kbd_id].buttons[button];
}

dynamic unsigned get_buf_len(unsigned kbd_id) {
	return kbds[kbd_id].buf_len;
}

dynamic unsigned read_buf(unsigned kbd_id, void* buf) {
	memcpy(buf, kbds[kbd_id].buf, get_buf_len(kbd_id));
	return 0;
}

// functions exclusively accessible to other devices

dynamic kbd_t* register_kbd(const char* name, update_callback_t update_callback, void* update_cb_param, unsigned set_default) {
	unsigned kbd_id = kbd_count++;

	kbds = realloc(kbds, kbd_count * sizeof *kbds);
	kbd_t* kbd = &kbds[kbd_id];
	
	memset(kbd, 0, sizeof *kbd);

	kbd->update_callback = update_callback;
	kbd->update_cb_param = update_cb_param;

	kbd->id = kbd_id;
	strncpy(kbd->name, name, sizeof kbd->name);

	if (set_default) {
		default_kbd_id = kbd_id;
	}
	
	return kbd;
}
