#include <aquabsd.alps.win/private.h>
#include <aquabsd.alps.win/functions.h>

typedef enum {
	CMD_CREATE              = 0x6377, // 'cw'
	CMD_DELETE              = 0x6477, // 'dw'

	CMD_SET_CAPTION         = 0x7363, // 'sc'
	CMD_GET_CAPTION         = 0x6763, // 'gc'

	CMD_GET_STATE           = 0x6773, // 'gs'

	CMD_REGISTER_CB         = 0x7263, // 'rc'
	CMD_LOOP                = 0x6C6F, // 'lo'

	CMD_CLOSE               = 0x636C, // 'cl'
	CMD_GRAB_FOCUS          = 0x6766, // 'gf'

	CMD_MODIFY              = 0x6D76, // 'mv'

	CMD_GET_X_POS           = 0x7870, // 'xp'
	CMD_GET_Y_POS           = 0x7970, // 'yp'

	CMD_GET_X_RES           = 0x7872, // 'xr'
	CMD_GET_Y_RES           = 0x7972, // 'yr'

	CMD_GET_WM_X_RES        = 0x7778, // 'wx'
	CMD_GET_WM_Y_RES        = 0x7779, // 'wy'

	// AQUA DWD protocol stuff (setters start with '-' (0x2D), getters start with '=' (0x3D))

	CMD_SUPPORTS_DWD        = 0x3D63, // '=s'

	CMD_SET_DWD_CLOSE_POS   = 0x2D78, // '-x'

	CMD_GET_DWD_CLOSE_POS_X = 0x3D78, // '=x'
	CMD_GET_DWD_CLOSE_POS_Y = 0x3D79, // '=y'

	// framebuffer stuff

	CMD_GET_FB              = 0x6662, // 'fb'
} cmd_t;

int load(void) {
	// aquabsd.alps.ftime

	ftime_device = kos_query_device(0, (uint64_t) "aquabsd.alps.ftime");

	if (ftime_device != -1) {
		aquabsd_alps_ftime_create = kos_load_device_function(ftime_device, "create");
		aquabsd_alps_ftime_draw   = kos_load_device_function(ftime_device, "draw"  );
		aquabsd_alps_ftime_swap   = kos_load_device_function(ftime_device, "swap"  );
		aquabsd_alps_ftime_done   = kos_load_device_function(ftime_device, "done"  );
		aquabsd_alps_ftime_delete = kos_load_device_function(ftime_device, "delete");
	}

	// aquabsd.alps.kbd

	kbd_device = kos_query_device(0, (uint64_t) "aquabsd.alps.kbd");

	if (kbd_device != -1) {
		aquabsd_alps_kbd_register_kbd = kos_load_device_function(kbd_device, "register_kbd");
		aquabsd_alps_kbd_x11_map      = kos_load_device_function(kbd_device, "x11_map");
	}

	// aquabsd.alps.mouse

	mouse_device = kos_query_device(0, (uint64_t) "aquabsd.alps.mouse");

	if (mouse_device != -1) {
		aquabsd_alps_mouse_register_mouse = kos_load_device_function(mouse_device, "register_mouse");
	}

	return 0;
}

uint64_t send(uint16_t _cmd, void* data) {
	cmd_t const cmd = _cmd;
	uint64_t* const args = data;

	if (cmd == CMD_CREATE) {
		unsigned const x_res = args[0];
		unsigned const y_res = args[1];

		return (uint64_t) create(x_res, y_res);
	}

	else if (cmd == CMD_DELETE) {
		win_t* const win = (void*) args[0];
		return delete(win);
	}

	else if (cmd == CMD_SET_CAPTION) {
		win_t* const win = (void*) args[0];
		const char* const caption = (void*) args[1];

		return set_caption(win, caption);
	}

	else if (cmd == CMD_GET_CAPTION) {
		win_t* const win = (void*) args[0];
		return (uint64_t) get_caption(win);
	}

	else if (cmd == CMD_GET_STATE) {
		win_t* const win = (void*) args[0];
		return get_state(win);
	}

	else if (cmd == CMD_REGISTER_CB) {
		win_t* const win = (void*) args[0];
		cb_t const type = args[1];

		uint64_t const cb = args[2];
		uint64_t const param = args[3];

		return register_cb(win, type, cb, param);
	}

	else if (cmd == CMD_LOOP) {
		win_t* const win = (void*) args[0];
		return loop(win);
	}

	else if (cmd == CMD_CLOSE) {
		win_t* const win = (void*) args[0];
		return close_win(win);
	}

	else if (cmd == CMD_GRAB_FOCUS) {
		win_t* const win = (void*) args[0];
		return grab_focus(win);
	}

	else if (cmd == CMD_MODIFY) {
		win_t* const win = (void*) args[0];

		float const x = *(float*) &args[1];
		float const y = *(float*) &args[2];

		unsigned const x_res = args[3];
		unsigned const y_res = args[4];

		return modify(win, x, y, x_res, y_res);
	}

	else if (cmd == CMD_GET_X_POS) {
		win_t* const win = (void*) args[0];

		float const x_pos = get_x_pos(win);
		return ((union { float _; uint64_t u; }) x_pos).u;
	}

	else if (cmd == CMD_GET_Y_POS) {
		win_t* const win = (void*) args[0];

		float const y_pos = get_y_pos(win);
		return ((union { float _; uint64_t u; }) y_pos).u;
	}

	else if (cmd == CMD_GET_X_RES) {
		win_t* const win = (void*) args[0];
		return get_x_res(win);
	}

	else if (cmd == CMD_GET_Y_RES) {
		win_t* const win = (void*) args[0];
		return get_y_res(win);
	}

	else if (cmd == CMD_GET_WM_X_RES) {
		win_t* const win = (void*) args[0];
		return get_wm_x_res(win);
	}

	else if (cmd == CMD_GET_WM_Y_RES) {
		win_t* const win = (void*) args[0];
		return get_wm_y_res(win);
	}

	// AQUA DWD protocol stuff

	else if (cmd == CMD_SUPPORTS_DWD) {
		win_t* const win = (void*) args[0];
		return supports_dwd(win);
	}

	else if (cmd == CMD_SET_DWD_CLOSE_POS) {
		win_t* const win = (void*) args[0];

		float const x = *(float*) &args[1];
		float const y = *(float*) &args[2];

		return set_dwd_close_pos(win, x, y);
	}

	else if (cmd == CMD_GET_DWD_CLOSE_POS_X) {
		win_t* const win = (void*) args[0];

		float const x = get_dwd_close_pos_x(win);
		return ((union { float f; uint64_t u; }) x).u;
	}

	else if (cmd == CMD_GET_DWD_CLOSE_POS_Y) {
		win_t* const win = (void*) args[0];

		float const y = get_dwd_close_pos_y(win);
		return ((union { float f; uint64_t u; }) y).u;
	}

	// framebuffer stuff

	else if (cmd == CMD_GET_FB) {
		win_t* const win = (void*) args[0];
		uint8_t const bpp = args[1];

		return (uint64_t) get_fb(win, bpp);
	}

	return -1;
}
