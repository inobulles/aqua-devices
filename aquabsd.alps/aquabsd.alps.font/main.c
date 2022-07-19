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

	else if (cmd == CMD_DRAW_FONT) {
		font_t* font = (font_t*) args[0];
		const char* string = (const char*) args[1];
		
		// TODO proper AXPN

		float red   = *(float*) &args[2];
		float green = *(float*) &args[3];
		float blue  = *(float*) &args[4];
		float alpha = *(float*) &args[5];

		uint64_t wrap_width  = (uint64_t) args[6];
		uint64_t wrap_height = (uint64_t) args[7];

		uint8_t** bitmap_reference = (uint8_t**) args[8];
		
		uint64_t* width_reference  = (uint64_t*) args[9];
		uint64_t* height_reference = (uint64_t*) args[10];

		return (uint64_t) draw_font(font, string, red, green, blue, alpha, wrap_width, wrap_height, bitmap_reference, width_reference, height_reference);
	}

	else if (cmd == CMD_FREE_FONT) {
		font_t* font = (font_t*) args[0];
		return (uint64_t) free_font(font);
	}

	return -1;
}
