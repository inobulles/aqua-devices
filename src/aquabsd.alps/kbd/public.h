#pragma once

#include <stdbool.h>
#include <stddef.h>

// definitions exclusively accessible to other devices

typedef struct aquabsd_alps_kbd_t aquabsd_alps_kbd_t; // forward declaration
typedef int (*aquabsd_alps_kbd_update_callback_t) (aquabsd_alps_kbd_t* kbd, void* param);

struct aquabsd_alps_kbd_t {
	unsigned available;

	aquabsd_alps_kbd_update_callback_t update_callback;
	void* update_cb_param;

	unsigned id;
	char name[256];

	#define AQUABSD_ALPS_KBD_BUTTON_COUNT 256
	bool buttons[AQUABSD_ALPS_KBD_BUTTON_COUNT];

	size_t buf_len;
	void* buf;

	size_t keys_len;
	const char** keys;
};

#if defined(__FreeBSD__)
	#define AQUABSD_ALPS_KBD_AQUABSD_CONSOLE_KBD
#endif

#define FP static __attribute__((unused))

FP unsigned (*aquabsd_alps_kbd_get_default_kbd_id) (void);
FP int (*aquabsd_alps_kbd_update_kbd) (unsigned kbd_id);

FP unsigned (*aquabsd_alps_kbd_poll_button) (unsigned kbd_id, unsigned button);

FP unsigned (*aquabsd_alps_kbd_get_buf_len) (unsigned kbd_id);
FP char* (*aquabsd_alps_kbd_get_buf) (unsigned kbd_id);

FP unsigned (*aquabsd_alps_kbd_get_keys_len) (unsigned kbd_id);
FP char const** (*aquabsd_alps_kbd_get_keys) (unsigned kbd_id);

FP aquabsd_alps_kbd_t* (*aquabsd_alps_kbd_register_kbd) (char const* name, aquabsd_alps_kbd_update_callback_t update_callback, void* update_cb_param, unsigned set_default);
FP char const* (*aquabsd_alps_kbd_x11_map) (int key);

#undef FP
