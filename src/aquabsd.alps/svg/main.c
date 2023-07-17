#include <aquabsd.alps/svg/private.h>
#include <aquabsd.alps/svg/functions.h>

typedef enum {
	CMD_LOAD_SVG = 0x6C73, // 'ls'
	CMD_DRAW_SVG = 0x6473, // 'ds'
	CMD_FREE_SVG = 0x6673, // 'fs'
} cmd_t;

uint64_t send(uint16_t _cmd, void* data) {
	cmd_t cmd = _cmd;
	uint64_t* args = data;

	if (cmd == CMD_LOAD_SVG) {
		const char* mem = (void*) args[0];
		return (uint64_t) load_svg(mem);
	}

	else if (cmd == CMD_DRAW_SVG) {
		svg_t* svg = (void*) args[0];
		uint64_t size = args[1];

		uint8_t** bitmap_reference = (void*) args[2];

		uint64_t* width_reference  = (void*) args[3];
		uint64_t* height_reference = (void*) args[4];

		return (uint64_t) draw_svg(svg, size, bitmap_reference, width_reference, height_reference);
	}

	else if (cmd == CMD_FREE_SVG) {
		svg_t* svg = (void*) args[0];
		return (uint64_t) free_svg(svg);
	}

	return -1;
}
