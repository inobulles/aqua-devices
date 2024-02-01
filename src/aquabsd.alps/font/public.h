#if !defined(__AQUABSD_ALPS_FONT)
#define __AQUABSD_ALPS_FONT

#include <stdint.h>
#include <stdbool.h>

// yes, it is insanely inelegant to have to use Cairo to render simple bitmaps

#include <pango/pangocairo.h>

typedef struct {
	PangoFontDescription* font_descr;

	// initial size may either be in points (non-absolute), or in device units (absolute, what aquaBSD uses)

	bool init_size_abs;
	double init_size;
} aquabsd_alps_font_t;

typedef enum {
	AQUABSD_ALPS_FONT_ALIGN_LEFT,
	AQUABSD_ALPS_FONT_ALIGN_RIGHT,
	AQUABSD_ALPS_FONT_ALIGN_CENTRE,
	AQUABSD_ALPS_FONT_ALIGN_JUSTIFY,
} aquabsd_alps_font_align_t;

typedef struct {
	// the 'dirty' field is set whenever layout settings change
	// this is so that we can wait until we actually need the layout object before generating it
	// this is for optimization purposes

	bool dirty;

	// pango stuff

	cairo_surface_t* surface;
	cairo_t* cairo;
	PangoLayout* layout;

	// mandatory fields

	aquabsd_alps_font_t* font;

	char* str;
	size_t len;

	// optional fields

	float r, g, b, a;
	uint64_t size; // in pixels, this is optional because, in the default case, the size will simply be that of the font
	uint64_t wrap_width, wrap_height;
	aquabsd_alps_font_align_t align;
	bool markup;

	// generated fields (after layout)

	int x_res, y_res;
} aquabsd_alps_font_text_t;

static aquabsd_alps_font_t* (*aquabsd_alps_font_load_font) (const char* path);
static int (*aquabsd_alps_font_free_font) (aquabsd_alps_font_t* font);

static aquabsd_alps_font_text_t* (*aquabsd_alps_font_create_text) (aquabsd_alps_font_t* font, const char* str);
static int (*aquabsd_alps_font_free_text) (aquabsd_alps_font_text_t* text);

static int (*aquabsd_alps_font_text_colour) (aquabsd_alps_font_text_t* text, float r, float g, float b, float a);
static int (*aquabsd_alps_font_text_size) (aquabsd_alps_font_text_t* text, uint64_t size);
static int (*aquabsd_alps_font_text_wrap) (aquabsd_alps_font_text_t* text, uint64_t wrap_width, uint64_t wrap_height);
static int (*aquabsd_alps_font_text_align) (aquabsd_alps_font_text_t* text, aquabsd_alps_font_align_t align);
static int (*aquabsd_alps_font_text_markup) (aquabsd_alps_font_text_t* text, bool markup);

static int (*aquabsd_alps_font_text_get_res) (aquabsd_alps_font_text_t* text, uint64_t* x_res_ref, uint64_t* y_res_ref);
static int (*aquabsd_alps_font_text_pos_to_i) (aquabsd_alps_font_text_t* text, uint64_t x, uint64_t y);
static int (*aquabsd_alps_font_text_i_to_pos) (aquabsd_alps_font_text_t* text, uint64_t i, uint64_t* x_ref, uint64_t* y_ref, uint64_t* width_ref, uint64_t* height_ref);

static int (*aquabsd_alps_font_draw_text) (aquabsd_alps_font_text_t* text, uint8_t** bmp_ref);

#endif
