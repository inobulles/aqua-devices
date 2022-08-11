#include <aquabsd.alps.wm/private.h>
#include <aquabsd.alps.wm/functions.h>

typedef enum {
	CMD_CREATE           = 0x6377, // 'cw'
	CMD_DELETE           = 0x6477, // 'dw'

	CMD_GET_ROOT_WIN     = 0x7277, // 'rw'

	CMD_GET_X_RES        = 0x7872, // 'xr'
	CMD_GET_Y_RES        = 0x7972, // 'yr'

	CMD_GET_CURSOR       = 0x6475, // 'cu'

	CMD_GET_CURSOR_XHOT  = 0x7868, // 'xh'
	CMD_GET_CURSOR_YHOT  = 0x7968, // 'yh'

	CMD_SET_NAME         = 0x736E, // 'sn'
	CMD_REGISTER_CB      = 0x7263, // 'rc'

	CMD_MAKE_COMPOSITING = 0x6D63, // 'mc'

	// provider stuff

	CMD_PROVIDER_COUNT   = 0x7064, // 'pc'

	CMD_PROVIDER_X       = 0x7078, // 'px'
	CMD_PROVIDER_Y       = 0x7079, // 'py'

	CMD_PROVIDER_X_RES   = 0x707A, // 'pz'
	CMD_PROVIDER_Y_RES   = 0x707B, // 'pw'
} cmd_t;

int load(void) {
	uint64_t win_device = kos_query_device(0, (uint64_t) "aquabsd.alps.win");

	if (win_device == -1) {
		return -1; // we cannot possibly function without the 'aquabsd.alps.win' device
	}

	aquabsd_alps_win_create_setup = kos_load_device_function(win_device, "create_setup");

	aquabsd_alps_win_set_caption = kos_load_device_function(win_device, "set_caption");
	aquabsd_alps_win_grab_focus = kos_load_device_function(win_device, "grab_focus");
	aquabsd_alps_win_delete = kos_load_device_function(win_device, "delete");

	return 0;
}

uint64_t send(uint16_t _cmd, void* data) {
	cmd_t cmd = _cmd;
	uint64_t* args = data;

	if (cmd == CMD_CREATE) {
		return (uint64_t) create();
	}

	else if (cmd == CMD_DELETE) {
		wm_t* wm = (void*) args[0];
		return delete(wm);
	}

	else if (cmd == CMD_GET_ROOT_WIN) {
		wm_t* wm = (void*) args[0];
		return (uint64_t) get_root_win(wm);
	}

	else if (cmd == CMD_GET_X_RES) {
		wm_t* wm = (void*) args[0];
		return (uint64_t) get_x_res(wm);
	}

	else if (cmd == CMD_GET_Y_RES) {
		wm_t* wm = (void*) args[0];
		return (uint64_t) get_y_res(wm);
	}

	else if (cmd == CMD_GET_CURSOR) {
		wm_t* wm = (void*) args[0];
		return (uint64_t) get_cursor(wm);
	}

	else if (cmd == CMD_GET_CURSOR_XHOT) {
		wm_t* wm = (void*) args[0];

		float xhot = get_cursor_xhot(wm);
		return ((union { float _; uint64_t u; }) xhot).u;
	}

	else if (cmd == CMD_GET_CURSOR_YHOT) {
		wm_t* wm = (void*) args[0];

		float yhot = get_cursor_yhot(wm);
		return ((union { float _; uint64_t u; }) yhot).u;
	}

	else if (cmd == CMD_SET_NAME) {
		wm_t* wm = (void*) args[0];
		const char* name = (void*) args[1];

		return set_name(wm, name);
	}

	else if (cmd == CMD_REGISTER_CB) {
		wm_t* wm = (void*) args[0];
		cb_t type = args[1];

		uint64_t cb = args[2];
		uint64_t param = args[3];

		return register_cb(wm, type, cb, param);
	}

	else if (cmd == CMD_MAKE_COMPOSITING) {
		wm_t* wm = (void*) args[0];
		return (uint64_t) make_compositing(wm);
	}

	// provider stuff

	else if (cmd == CMD_PROVIDER_COUNT) {
		wm_t* wm = (void*) args[0];
		return (uint64_t) provider_count(wm);
	}

	else if (cmd == CMD_PROVIDER_X) {
		wm_t* wm = (void*) args[0];
		unsigned id = args[1];

		return (uint64_t) provider_x(wm, id);
	}

	else if (cmd == CMD_PROVIDER_Y) {
		wm_t* wm = (void*) args[0];
		unsigned id = args[1];

		return (uint64_t) provider_y(wm, id);
	}

	else if (cmd == CMD_PROVIDER_X_RES) {
		wm_t* wm = (void*) args[0];
		unsigned id = args[1];

		return (uint64_t) provider_x_res(wm, id);
	}

	else if (cmd == CMD_PROVIDER_Y_RES) {
		wm_t* wm = (void*) args[0];
		unsigned id = args[1];

		return (uint64_t) provider_y_res(wm, id);
	}

	return -1;
}
