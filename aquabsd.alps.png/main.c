#include <aquabsd.alps.png/private.h>
#include <aquabsd.alps.png/functions.h>

#define CMD_LOAD 0x6C64 // 'ld'
#define CMD_DRAW 0x6477 // 'dw'
#define CMD_FREE 0x6665 // 'fe'

uint64_t send(uint16_t command, void* data) {
	uint64_t* arguments = data;

	if (command == CMD_LOAD) {
		const char* path = (void*) arguments[0];
		return (uint64_t) load_png(path);
	}

	else if (command == CMD_DRAW) {
		png_t* png = (png_t*) arguments[0];

		uint8_t** bitmap_reference = (void*) arguments[1];
		uint64_t* bpp_reference    = (void*) arguments[2];
		
		uint64_t* width_reference  = (void*) arguments[3];
		uint64_t* height_reference = (void*) arguments[4];

		return (uint64_t) draw_png(png, bitmap_reference, bpp_reference, width_reference, height_reference);
	}

	else if (command == CMD_FREE) {
		png_t* png = (png_t*) arguments[0];
		return (uint64_t) free_png(png);
	}

	return -1;
}
