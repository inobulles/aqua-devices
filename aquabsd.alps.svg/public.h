#if !defined(__AQUABSD_ALPS_SVG)
#define __AQUABSD_ALPS_SVG

#include <stdint.h>

// listen here RSVG, if you're not going to give me any facilities in your stupid header to check compile-time which version of your shitty library I'm using, I might aswell just use your dumb deprecated functions whether you like it or not, and not put in the effort to update my code to use your "new" API
// (sorry I went overboard your project is cool but it's a bit frustrating)

#if !defined(AQUABSD_ALPS_SVG_NEW_RSVG)
	#define RSVG_DISABLE_DEPRECATION_WARNINGS
#endif

#include <librsvg/rsvg.h>

typedef struct {
	RsvgHandle* handle;
	double width, height;
} aquabsd_alps_svg_t;

static aquabsd_alps_svg_t* (*aquabsd_alps_svg_load_svg) (const char* path);
static aquabsd_alps_svg_t* (*aquabsd_alps_svg_load_svg_str) (const char* str);
static int (*aquabsd_alps_svg_free_svg) (aquabsd_alps_svg_t* svg);
static int (*aquabsd_alps_svg_draw_svg) (aquabsd_alps_svg_t* svg, uint64_t size, uint8_t** bitmap_reference, uint64_t* width_reference, uint64_t* height_reference);

#endif
