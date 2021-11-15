#include <aquabsd.alps.ogl/private.h>
#include <aquabsd.alps.ogl/functions.h>

#define CMD_CREATE_WINDOW  0x6377 // 'cw'
#define CMD_CREATE_WM      0x636D // 'cm'

#define CMD_DELETE_CONTEXT 0x6463 // 'dc'

#define CMD_REGISTER_CB    0x7263 // 'rc'
#define CMD_LOOP           0x6C6F // 'lo'

#define CMD_GET_FUNCTION   0x6766 // 'gf'

int load(
	uint64_t (*_kos_query_device) (uint64_t, uint64_t name),
	void* (*_kos_load_device_function) (uint64_t device, const char* name),
	uint64_t (*_kos_callback) (uint64_t callback, int argument_count, ...)) {

	kos_query_device = _kos_query_device;
	kos_load_device_function = _kos_load_device_function;
	kos_callback = _kos_callback;

	return 0;
}

uint64_t send(uint16_t command, void* data) {
	uint64_t* arguments = (uint64_t*) data;

	if (command == CMD_CREATE_WINDOW) {
		unsigned x_res = arguments[0];
		unsigned y_res = arguments[1];

		return (uint64_t) create_window(x_res, y_res);
	}

	else if (command == CMD_CREATE_WM) {
		return (uint64_t) create_wm();
	}

	else if (command == CMD_DELETE_CONTEXT) {
		context_t* context = (void*) arguments[0];
		return delete_context(context);
	}

	else if (command == CMD_REGISTER_CB) {
		context_t* context = (void*) arguments[0];
		cb_t type = arguments[1];

		uint64_t cb = arguments[2];
		uint64_t param = arguments[3];

		return register_cb(context, type, cb, param);
	}

	else if (command == CMD_LOOP) {
		context_t* context = (void*) arguments[0];
		return loop(context);
	}

	else if (command == CMD_GET_FUNCTION) {
		context_t* context = (void*) arguments[0];
		const char* name = (void*) arguments[1];

		return (uint64_t) get_function(context, name);
	}

	return -1;
}
