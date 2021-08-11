#if !defined(__AQUABSD_ALPS_FONT__)
#define __AQUABSD_ALPS_FONT__

#include <stdint.h>

// yes, it is insanely inelegant to have to use Cairo to render simple bitmaps
// blame Pango, it's a fault of their own intelligence, not mine

#include <pango/pangocairo.h>

typedef struct {
	PangoFontDescription* font_description;
} font_t;

font_t* (*aquabsd_alps_font_load_font) (const char* path);
int (*aquabsd_alps_font_free_font) (font_t* font);
int (*aquabsd_alps_font_draw_font) (font_t* font, const char* string, float red, float green, float blue, float alpha, int64_t wrap_width, int64_t wrap_height, uint8_t** bitmap_reference, uint64_t* width_reference, uint64_t* height_reference);

#endif
