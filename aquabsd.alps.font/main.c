#include <aquabsd.alps.font/private.h>
#include <aquabsd.alps.font/functions.h>

#define CMD_LOAD_FONT 0x6C66 // 'lf'
#define CMD_DRAW_FONT 0x6466 // 'df'
#define CMD_FREE_FONT 0x6666 // 'ff'

uint64_t send(uint16_t command, void* data) {
	uint64_t* arguments = (uint64_t*) data;

	if (command == CMD_LOAD_FONT) {
		const char* path = (const char*) arguments[0];
		return (uint64_t) load_font(path);
	}

	else if (command == CMD_DRAW_FONT) {
		font_t* font = (font_t*) arguments[0];
		const char* string = (const char*) arguments[1];
		
		// TODO proper AXPN

		float red   = *(float*) &arguments[2];
		float green = *(float*) &arguments[3];
		float blue  = *(float*) &arguments[4];
		float alpha = *(float*) &arguments[5];

		uint64_t wrap_width  = (uint64_t) arguments[6];
		uint64_t wrap_height = (uint64_t) arguments[7];

		uint8_t** bitmap_reference = (uint8_t**) arguments[8];
		
		uint64_t* width_reference  = (uint64_t*) arguments[9];
		uint64_t* height_reference = (uint64_t*) arguments[10];

		return (uint64_t) draw_font(font, string, red, green, blue, alpha, wrap_width, wrap_height, bitmap_reference, width_reference, height_reference);
	}

	else if (command == CMD_FREE_FONT) {
		font_t* font = (font_t*) arguments[0];
		return (uint64_t) free_font(font);
	}

	return -1;
}
