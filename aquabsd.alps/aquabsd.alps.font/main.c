#include <aquabsd.alps.font/private.h>
#include <aquabsd.alps.font/functions.h>

typedef enum {
	CMD_LOAD_FONT = 0x6C66, // 'lf'
	CMD_DRAW_FONT = 0x6466, // 'df'
	CMD_FREE_FONT = 0x6666, // 'ff'
} cmd_t;

uint64_t send(uint16_t _cmd, void* data) {
	cmd_t cmd = _cmd;
	uint64_t* args = (uint64_t*) data;

	if (cmd == CMD_LOAD_FONT) {
		const char* path = (const char*) args[0];
		return (uint64_t) load_font(path);
	}

	else if (cmd == CMD_FREE_FONT) {
		font_t* font = (font_t*) args[0];
		return (uint64_t) free_font(font);
	}

	// TODO rest duh

	return -1;
}
