#include <aquabsd.alps.png/private.h>
#include <aquabsd.alps.png/functions.h>

typedef enum {
	CMD_LOAD     = 0x6C64, // 'ld'
	CMD_LOAD_PTR = 0x6C70, // 'lp'
	CMD_DRAW     = 0x6477, // 'dw'
	CMD_FREE     = 0x6665, // 'fe'
} cmd_t;

uint64_t send(uint16_t _cmd, void* data) {
	cmd_t cmd = _cmd;
	uint64_t* args = data;

	if (cmd == CMD_LOAD) {
		const char* path = (void*) args[0];
		return (uint64_t) load_png(path);
	}

	else if (cmd == CMD_LOAD_PTR) {
		void* ptr = (void*) args[0];
		return (uint64_t) load_png_ptr(ptr);
	}

	else if (cmd == CMD_DRAW) {
		png_t* png = (void*) args[0];

		uint8_t** bitmap_reference = (void*) args[1];
		uint64_t* bpp_reference    = (void*) args[2];
		
		uint64_t* width_reference  = (void*) args[3];
		uint64_t* height_reference = (void*) args[4];

		return (uint64_t) draw_png(png, bitmap_reference, bpp_reference, width_reference, height_reference);
	}

	else if (cmd == CMD_FREE) {
		png_t* png = (void*) args[0];
		return (uint64_t) free_png(png);
	}

	return -1;
}
