// TODO for resizing, investigate if how picom does it is any good ('src/backend/gl/gl_common.c' line 1606)

#include <aquabsd.alps.ogl/private.h>
#include <aquabsd.alps.ogl/functions.h>

typedef enum {
	CMD_CREATE       = 0x6363, // 'cc'
	CMD_DELETE       = 0x6364, // 'dc'

	CMD_GET_FUNCTION = 0x6766, // 'gf'
	CMD_BIND_WIN_TEX = 0x6277, // 'bw'
} cmd_t;

int load(
    uint64_t (*_kos_query_device) (uint64_t, uint64_t name),
    void* (*_kos_load_device_function) (uint64_t device, const char* name),
    uint64_t (*_kos_callback) (uint64_t callback, int argument_count, ...)) {

    kos_query_device = _kos_query_device;
    kos_load_device_function = _kos_load_device_function;
	kos_callback = _kos_callback;

    win_device = kos_query_device(0, (uint64_t) "aquabsd.alps.win");

	if (win_device != -1) {
    	aquabsd_alps_win_register_dev_cb = kos_load_device_function(win_device, "register_dev_cb");
		aquabsd_alps_win_get_draw_win = kos_load_device_function(win_device, "get_draw_win");
	}

    return 0;
}

uint64_t send(uint16_t _cmd, void* data) {
	cmd_t cmd = _cmd;
	uint64_t* args = data;

	if (cmd == CMD_CREATE) {
		context_type_t type = args[0];
		context_t* context = NULL;

		if (type == CONTEXT_TYPE_WIN && win_device != -1) {
			aquabsd_alps_win_t* win = (void*) args[1];
			context = create_win_context(win);
		}

		return (uint64_t) context;
	}

	else if (cmd == CMD_DELETE) {
		context_t* context = (void*) args[0];
		return delete(context);
	}

	else if (cmd == CMD_GET_FUNCTION) {
		context_t* context = (void*) args[0];
		const char* name = (void*) args[1];

		return (uint64_t) get_function(context, name);
	}

	else if (cmd == CMD_BIND_WIN_TEX) {
		context_t* context = (void*) args[0];
		aquabsd_alps_win_t* win = (void*) args[1];

		return bind_win_tex(context, win);
	}

	return -1;
}
