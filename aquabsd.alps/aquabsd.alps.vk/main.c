#include <aquabsd.alps.vk/private.h>
#include <aquabsd.alps.vk/functions.h>

typedef enum {
	CMD_CREATE       = 0x6363, // 'cc'
	CMD_DELETE       = 0x6364, // 'dc'

	CMD_GET_FUNCTION = 0x6766, // 'gf'
} cmd_t;

int load(void) {
	// aquabsd.alps.ftime

	ftime_device = kos_query_device(0, (uint64_t) "aquabsd.alps.ftime");

	if (ftime_device != (uint64_t) -1) {
		aquabsd_alps_ftime_create = kos_load_device_function(ftime_device, "create");
		aquabsd_alps_ftime_draw   = kos_load_device_function(ftime_device, "draw"  );
		aquabsd_alps_ftime_swap   = kos_load_device_function(ftime_device, "swap"  );
		aquabsd_alps_ftime_done   = kos_load_device_function(ftime_device, "done"  );
		aquabsd_alps_ftime_delete = kos_load_device_function(ftime_device, "delete");
	}

	// aquabsd.alps.win

    win_device = kos_query_device(0, (uint64_t) "aquabsd.alps.win");

	if (win_device != (uint64_t) -1) {
    	aquabsd_alps_win_register_dev_cb = kos_load_device_function(win_device, "register_dev_cb");
		aquabsd_alps_win_get_draw_win = kos_load_device_function(win_device, "get_draw_win");
	}

    return 0;
}

uint64_t send(uint16_t _cmd, void* data) {
	cmd_t cmd = _cmd;
	uint64_t* args = data;

	if (cmd == CMD_CREATE) {
		context_type_t const type = args[0];
		context_t* context = NULL;

		if (type == CONTEXT_TYPE_WIN && win_device != (uint64_t) -1) {
			aquabsd_alps_win_t* const win = (void*) args[1];
			char const* const name = (void*) args[2];

			size_t const ver_major = args[3];
			size_t const ver_minor = args[4];
			size_t const ver_patch = args[5];

			context = create_win_context(win, name, ver_major, ver_minor, ver_patch);
		}

		return (uint64_t) context;
	}

	else if (cmd == CMD_DELETE) {
		context_t* context = (void*) args[0];
		free_context(context);

		return 0;
	}

	else if (cmd == CMD_GET_FUNCTION) {
		context_t* context = (void*) args[0];
		const char* name = (void*) args[1];

		return (uint64_t) get_func(context, name);
	}

	return -1;
}
