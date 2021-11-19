#include <aquabsd.alps.win/private.h>
#include <aquabsd.alps.win/functions.h>

#define CMD_CREATE      0x6377 // 'cw'
#define CMD_DELETE      0x6477 // 'cd'

#define CMD_SET_CAPTION 0x7363 // 'sc'

#define CMD_REGISTER_CB 0x7263 // 'rc'
#define CMD_LOOP        0x6C6F // 'lo'

int load(
	uint64_t (*_kos_query_device) (uint64_t, uint64_t name),
	void* (*_kos_load_device_function) (uint64_t device, const char* name),
	uint64_t (*_kos_callback) (uint64_t callback, int argument_count, ...)) {

	kos_query_device = _kos_query_device;
	kos_load_device_function = _kos_load_device_function;
	kos_callback = _kos_callback;

	mouse_device = kos_query_device(0, (uint64_t) "aquabsd.alps.mouse");

	if (mouse_device != -1) {
		aquabsd_alps_mouse_register_mouse = kos_load_device_function(mouse_device, "register_mouse");
	}

	return 0;
}

uint64_t send(uint16_t command, void* data) {
	uint64_t* arguments = (uint64_t*) data;

	if (command == CMD_CREATE) {
		unsigned x_res = arguments[0];
		unsigned y_res = arguments[1];

		return (uint64_t) create(x_res, y_res);
	}

	else if (command == CMD_DELETE) {
		win_t* win = (void*) arguments[0];
		return delete(win);
	}

	else if (command == CMD_SET_CAPTION) {
		win_t* win = (void*) arguments[0];
		const char* caption = (void*) arguments[1];

		return set_caption(win, caption);
	}	

	else if (command == CMD_REGISTER_CB) {
		win_t* win = (void*) arguments[0];
		cb_t type = arguments[1];

		uint64_t cb = arguments[2];
		uint64_t param = arguments[3];

		return register_cb(win, type, cb, param);
	}

	else if (command == CMD_LOOP) {
		win_t* win = (void*) arguments[0];
		return loop(win);
	}

	return -1;
}
