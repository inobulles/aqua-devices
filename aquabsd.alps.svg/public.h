#if !defined(__AQUABSD_ALPS_SVG)
#define __AQUABSD_ALPS_SVG

#include <stdint.h>
#include <librsvg/rsvg.h>

typedef struct {
	RsvgHandle* handle;
	RsvgDimensionData dimensions;
} aquabsd_alps_svg_t;

static aquabsd_alps_svg_t* (*aquabsd_alps_svg_load_svg) (const char* path);
static int (*aquabsd_alps_svg_free_svg) (aquabsd_alps_svg_t* svg);
static int (*aquabsd_alps_svg_draw_svg) (aquabsd_alps_svg_t* svg, uint64_t size, uint8_t** bitmap_reference, uint64_t* width_reference, uint64_t* height_reference);

#endif
