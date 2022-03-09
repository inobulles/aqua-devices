#if !defined(__AQUABSD_ALPS_PNG)
#define __AQUABSD_ALPS_PNG

#include <stdint.h>
#include <string.h>

#include <png.h>

typedef enum {
	AQUABSD_ALPS_PNG_STREAM_KIND_FP,
	AQUABSD_ALPS_PNG_STREAM_KIND_PTR,
} aquabsd_alps_png_stream_kind_t;

typedef struct {
	void* ptr;
} aquabsd_alps_png_stream_t;

typedef struct {
	aquabsd_alps_png_stream_kind_t stream_kind;

	union {
		FILE* fp;
		aquabsd_alps_png_stream_t stream;
	};

	png_structp png;
	png_infop info;

	unsigned width, height;
	png_byte colour_type, bit_depth;
	int number_of_passes;

	uint8_t* bitmap;

	size_t bytes;
	size_t row_bytes;
} aquabsd_alps_png_t;

static aquabsd_alps_png_t* (*aquabsd_alps_png_load) (const char* path);
static int (*aquabsd_alps_png_free) (aquabsd_alps_png_t* png);
static int (*aquabsd_alps_png_draw) (aquabsd_alps_png_t* png, uint8_t** bitmap_reference, uint64_t* bpp_reference, uint64_t* width_reference, uint64_t* height_reference);

#endif
