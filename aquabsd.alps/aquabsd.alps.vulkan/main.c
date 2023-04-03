#include <aquabsd.alps.vulkan/private.h>
#include <aquabsd.alps.vulkan/functions.h>

typedef enum {
	CMD_EMPTY
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

	(void) cmd;
	(void) args;

	return -1;
}
