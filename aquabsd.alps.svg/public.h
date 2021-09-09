#if !defined(__AQUABSD_ALPS_SVG)
#define __AQUABSD_ALPS_SVG

// TODO

#include <librsvg/rsvg.h>

typedef struct {
	RsvgHandle* handle;
	RsvgDimensionData dimensions;
} aquabsd_alps_svg_t;

#endif