#if !defined(__AQUABSD_ALPS_FONT)
#define __AQUABSD_ALPS_FONT

#include <stdint.h>
#include <stdbool.h>

// yes, it is insanely inelegant to have to use Cairo to render simple bitmaps

#include <pango/pangocairo.h>

typedef struct {
	PangoFontDescription* font_description;
} aquabsd_alps_font_t;

typedef enum {
	AQUABSD_ALPS_FONT_ALIGN_LEFT,
	AQUABSD_ALPS_FONT_ALIGN_RIGHT,
	AQUABSD_ALPS_FONT_ALIGN_CENTRE,
	AQUABSD_ALPS_FONT_ALIGN_JUSTIFY,
} aquabsd_alps_font_align_t;

typedef struct {
	PangoLayout* layout;

	// mandatory fields

	aquabsd_alps_font_t* font;

	char* str;
	size_t len;

	float red, green, blue, alpha;

	// optional fields

	uint64_t size; // in pixels, this is optional because, in the default case, the size will simply be that of the font
	uint64_t wrap_width, wrap_height;
	aquabsd_alps_font_align_t align;
	bool markup;
} aquabsd_alps_font_text_t;

aquabsd_alps_font_t* (*aquabsd_alps_font_load_font) (const char* path);
int (*aquabsd_alps_font_free_font) (aquabsd_alps_font_t* font);

aquabsd_alps_font_text_t* (*aquabsd_alps_font_create_text) (aquabsd_alps_font_t* font, const char* str, float red, float green, float blue, float alpha);
int (*aquabsd_alps_font_free_text) (aquabsd_alps_font_text_t* text);

int (*aquabsd_alps_font_text_size) (aquabsd_alps_font_text_t* text, uint64_t size);
int (*aquabsd_alps_font_text_wrapping) (aquabsd_alps_font_text_t* text, uint64_t wrap_width, uint64_t wrap_height);
int (*aquabsd_alps_font_text_align) (aquabsd_alps_font_text_t* text, aquabsd_alps_font_align_t align);
int (*aquabsd_alps_font_text_markup) (aquabsd_alps_font_text_t* text, bool markup);

int (*aquabsd_alps_font_text_pos_to_i) (aquabsd_alps_font_text_t* text, uint64_t x, uint64_t y);
int (*aquabsd_alps_font_text_i_to_pos) (aquabsd_alps_font_text_t* text, uint64_t i, uint64_t x_ref, uint64_t y_ref, uint64_t width_ref, uint64_t height_ref);

int (*aquabsd_alps_font_draw_text) (aquabsd_alps_font_text_t* text, uint8_t** bitmap_ref, uint64_t* width_ref, uint64_t* height_ref);

#endif
