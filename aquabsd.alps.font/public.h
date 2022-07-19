#if !defined(__AQUABSD_ALPS_FONT)
#define __AQUABSD_ALPS_FONT

#include <stdint.h>

// yes, it is insanely inelegant to have to use Cairo to render simple bitmaps

#include <pango/pangocairo.h>

typedef struct {
	PangoFontDescription* font_description;
} aquabsd_alps_font_t;

aquabsd_alps_font_t* (*aquabsd_alps_font_load_font) (const char* path);
int (*aquabsd_alps_font_free_font) (aquabsd_alps_font_t* font);
int (*aquabsd_alps_font_draw_font) (aquabsd_alps_font_t* font, const char* string, float red, float green, float blue, float alpha, uint64_t wrap_width, uint64_t wrap_height, uint8_t** bitmap_reference, uint64_t* width_reference, uint64_t* height_reference);

#endif
