#include <aquabsd.alps.svg/private.h>
#include <aquabsd.alps.svg/functions.h>

#define CMD_LOAD_SVG 0x6C73 // 'ls'
#define CMD_DRAW_SVG 0x6473 // 'ds'
#define CMD_FREE_SVG 0x6673 // 'fs'

uint64_t send(uint16_t command, void* data) {
	uint64_t* arguments = (uint64_t*) data;

	if (command == CMD_LOAD_SVG) {
		const char* path = (const char*) arguments[0];
		return (uint64_t) load_svg(path);
	}

	else if (command == CMD_DRAW_SVG) {
		svg_t* svg = (svg_t*) arguments[0];
		uint64_t size = arguments[1];

		uint8_t** bitmap_reference = (uint8_t**) arguments[2];
		
		uint64_t* width_reference  = (uint64_t*) arguments[3];
		uint64_t* height_reference = (uint64_t*) arguments[4];

		return (uint64_t) draw_svg(svg, size, bitmap_reference, width_reference, height_reference);
	}

	else if (command == CMD_FREE_SVG) {
		svg_t* svg = (svg_t*) arguments[0];
		return (uint64_t) free_svg(svg);
	}

	return -1;
}