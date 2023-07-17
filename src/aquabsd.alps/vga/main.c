#include <aquabsd.alps/vga/private.h>
#include <aquabsd.alps/vga/functions.h>

typedef enum {
	CMD_MODE_COUNT  = 0x6D63, // 'mc'
	CMD_MODES       = 0x6D64, // 'md'

	CMD_SET_MODE    = 0x736D, // 'sm'
	CMD_FRAMEBUFFER = 0x6662, // 'fb'

	CMD_FLIP        = 0x666C, // 'fl'

	CMD_RESET       = 0x7672, // 'vr'
} cmd_t;

uint64_t send(uint16_t _cmd, void* data) {
	cmd_t cmd = _cmd;
	uint64_t* args = data;

	if (cmd == CMD_MODE_COUNT) {
		return get_mode_count();
	}

	else if (cmd == CMD_MODES) {
		return (uint64_t) get_modes();
	}

	else if (cmd == CMD_SET_MODE) {
		video_mode_t* mode = (void*) args[0];
		return set_mode(mode);
	}

	else if (cmd == CMD_FRAMEBUFFER) {
		return (uint64_t) get_framebuffer();
	}

	else if (cmd == CMD_FLIP) {
		return flip();
	}

	else if (cmd == CMD_RESET) {
		return reset();
	}

	return -1;
}
