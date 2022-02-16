#if !defined(__AQUABSD_ALPS_PNG)
#define __AQUABSD_ALPS_PNG

#include <stdint.h>
#include <string.h>

#include <png.h>

typedef struct {
	void* ptr;
} aquabsd_alps_png_stream_t;

typedef struct {
	FILE* fp;
	aquabsd_alps_png_stream_t stream;

	png_structp png;
	png_infop info;

	unsigned width, height;
	png_byte colour_type, bit_depth;
	int number_of_passes;
} aquabsd_alps_png_t;

static aquabsd_alps_png_t* (*aquabsd_alps_png_load) (const char* path);
static int (*aquabsd_alps_png_free) (aquabsd_alps_png_t* png);
static int (*aquabsd_alps_png_draw) (aquabsd_alps_png_t* png, uint8_t** bitmap_reference, uint64_t* bpp_reference, uint64_t* width_reference, uint64_t* height_reference);

#endif
