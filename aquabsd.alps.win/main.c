#include <aquabsd.alps.win/private.h>
#include <aquabsd.alps.win/functions.h>

typedef enum {
	CMD_CREATE       = 0x6377, // 'cw'
	CMD_DELETE       = 0x6477, // 'dw'

	CMD_SET_CAPTION  = 0x7363, // 'sc'
	CMD_GET_CAPTION  = 0x6763, // 'gc'

	CMD_GET_STATE    = 0x6773, // 'gs'

	CMD_REGISTER_CB  = 0x7263, // 'rc'
	CMD_LOOP         = 0x6C6F, // 'lo'

	CMD_CLOSE        = 0x636C, // 'cl'
	CMD_GRAB_FOCUS   = 0x6667, // 'gf'

	CMD_MODIFY       = 0x6D76, // 'mv'

	CMD_GET_X_POS    = 0x7870, // 'xp'
	CMD_GET_Y_POS    = 0x7970, // 'yp'

	CMD_GET_X_RES    = 0x7872, // 'xr'
	CMD_GET_Y_RES    = 0x7972, // 'yr'

	CMD_GET_WM_X_RES = 0x7778, // 'wx'
	CMD_GET_WM_Y_RES = 0x7779, // 'wy'
} cmd_t;

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

	kbd_device = kos_query_device(0, (uint64_t) "aquabsd.alps.kbd");

	if (kbd_device != -1) {
		aquabsd_alps_kbd_register_kbd = kos_load_device_function(kbd_device, "register_kbd");
	}

	return 0;
}

uint64_t send(uint16_t _cmd, void* data) {
	cmd_t cmd = _cmd;
	uint64_t* args = data;

	if (cmd == CMD_CREATE) {
		unsigned x_res = args[0];
		unsigned y_res = args[1];

		return (uint64_t) create(x_res, y_res);
	}

	else if (cmd == CMD_DELETE) {
		win_t* win = (void*) args[0];
		return delete(win);
	}

	else if (cmd == CMD_SET_CAPTION) {
		win_t* win = (void*) args[0];
		const char* caption = (void*) args[1];

		return set_caption(win, caption);
	}

	else if (cmd == CMD_GET_CAPTION) {
		win_t* win = (void*) args[0];
		return (uint64_t) get_caption(win);
	}

	else if (cmd == CMD_GET_STATE) {
		win_t* win = (void*) args[0];
		return get_state(win);
	}

	else if (cmd == CMD_REGISTER_CB) {
		win_t* win = (void*) args[0];
		cb_t type = args[1];

		uint64_t cb = args[2];
		uint64_t param = args[3];

		return register_cb(win, type, cb, param);
	}

	else if (cmd == CMD_LOOP) {
		win_t* win = (void*) args[0];
		return loop(win);
	}

	else if (cmd == CMD_CLOSE) {
		win_t* win = (void*) args[0];
		return close_win(win);
	}

	else if (cmd == CMD_GRAB_FOCUS) {
		win_t* win = (void*) args[0];
		return grab_focus(win);
	}

	else if (cmd == CMD_MODIFY) {
		win_t* win = (void*) args[0];

		float x = *(float*) &args[1];
		float y = *(float*) &args[2];

		unsigned x_res = args[3];
		unsigned y_res = args[4];

		return modify(win, x, y, x_res, y_res);
	}

	else if (cmd == CMD_GET_X_POS) {
		win_t* win = (void*) args[0];
		
		float x_pos = get_x_pos(win);
		return *(uint64_t*) &x_pos;
	}

	else if (cmd == CMD_GET_Y_POS) {
		win_t* win = (void*) args[0];

		float y_pos = get_y_pos(win);
		return *(uint64_t*) &y_pos;
	}

	else if (cmd == CMD_GET_X_RES) {
		win_t* win = (void*) args[0];
		return get_x_res(win);
	}

	else if (cmd == CMD_GET_Y_RES) {
		win_t* win = (void*) args[0];
		return get_y_res(win);
	}

	else if (cmd == CMD_GET_WM_X_RES) {
		win_t* win = (void*) args[0];
		return get_wm_x_res(win);
	}

	else if (cmd == CMD_GET_WM_Y_RES) {
		win_t* win = (void*) args[0];
		return get_wm_y_res(win);
	}

	return -1;
}
