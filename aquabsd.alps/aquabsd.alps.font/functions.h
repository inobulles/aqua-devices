// TODO take a look at 'PangoContext'
//      apparently, what I'm doing right now is inefficient
//      cf. https://developer.gimp.org/api/2.0/pango/pango-Cairo-Rendering.html#pango-cairo-create-layout
//      not sure I understand all of this, but maakt niet uit

#include <umber.h>
#define UMBER_COMPONENT "aquabsd.alps.font"

// TODO make 'path' actually be the path somehow

dynamic font_t* load_font(const char* path) {
	LOG_INFO("Load font \"%s\"", path)

	PangoFontDescription* font_description = pango_font_description_from_string(path);

	if (!font_description) {
		LOG_ERROR("Could not load font \"%s\"", path);
		return NULL;
	}

	font_t* font = calloc(1, sizeof *font);
	font->font_description = font_description;

	LOG_SUCCESS("Loaded font: %p", font)

	return font;
}

dynamic int free_font(font_t* font) {
	pango_font_description_free(font->font_description);
	return 0;
}

// text object functions

dynamic int text_colour(text_t* text, float r, float g, float b, float a);
dynamic int text_size  (text_t* text, uint64_t size);
dynamic int text_wrap  (text_t* text, uint64_t wrap_width, uint64_t wrap_height);
dynamic int text_align (text_t* text, align_t align);
dynamic int text_markup(text_t* text, bool markup);

dynamic text_t* create_text(font_t* font, const char* str) {
	text_t* text = calloc(1, sizeof *text);
	text->dirty = true;

	// set mandatory fields

	text->font = font;

	text->str = strdup(str);
	text->len = strlen(str);

	// set defaults

	text_colour(text, 1.0, 1.0, 1.0, 1.0);
	text_size(text, 0);
	text_wrap(text, 0, 0);
	text_align(text, ALIGN_CENTRE);
	text_markup(text, false);

	return text;
}

dynamic int free_text(text_t* text) {
	if (text->str) {
		free(text->str);
	}

	if (text->surface) {
		cairo_surface_destroy(text->surface);
	}

	if (text->layout) {
		g_object_unref(text->layout);
	}

	if (text->cairo) {
		cairo_destroy(text->cairo);
	}

	free(text);
	return 0;
}

dynamic int text_colour(text_t* text, float r, float g, float b, float a) {
	text->r = r;
	text->g = g;
	text->b = b;
	text->a = a;

	text->dirty = true;
	return 0;
}

dynamic int text_size(text_t* text, uint64_t size) {
	text->size = size;

	text->dirty = true;
	return 0;
}

dynamic int text_wrap(text_t* text, uint64_t width, uint64_t height) {
	text->wrap_width  = width;
	text->wrap_height = height;

	text->dirty = true;
	return 0;
}

dynamic int text_align(text_t* text, align_t align) {
	text->align = align;

	text->dirty = true;
	return 0;
}

dynamic int text_markup(text_t* text, bool markup) {
	text->markup = markup;

	text->dirty = true;
	return 0;
}

static void gen_layout(text_t* text) {
	if (!text->dirty) {
		return;
	}

	text->dirty = false;

	// create dummy cairo surface if there isn't already one
	// this is simply for getting font metrics which we can't get otherwise (because the Pango API is a little quirky like that)

	if (!text->surface || !text->cairo) {
		text->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
		text->cairo = cairo_create(text->surface);
	}

	if (!text->layout) {
		text->layout = pango_cairo_create_layout(text->cairo);
	}

	pango_layout_set_font_description(text->layout, text->font->font_description);

	#define MAX_WIDTH  0x2000
	#define MAX_HEIGHT 0x2000

	if (text->wrap_width > MAX_WIDTH || !text->wrap_width) {
		text->wrap_width = MAX_WIDTH;
	}

	if (text->wrap_height > MAX_HEIGHT || !text->wrap_height) {
		text->wrap_height = MAX_HEIGHT;
	}

	pango_layout_set_width (text->layout, text->wrap_width  * PANGO_SCALE); // idk why 'PANGO_PIXELS()' doesn't work here
	pango_layout_set_height(text->layout, text->wrap_height * PANGO_SCALE);

	pango_layout_set_wrap(text->layout, PANGO_WRAP_WORD_CHAR); // TODO should this be settable?

	void (*layout_fn) (PangoLayout* layout, const char* str, int len) = pango_layout_set_text;

	if (text->markup) {
		layout_fn = pango_layout_set_markup;
	}

	layout_fn(text->layout, text->str, text->len);

	pango_cairo_update_layout(text->cairo, text->layout);
	pango_layout_get_size(text->layout, &text->x_res, &text->y_res);

	text->x_res /= PANGO_SCALE;
	text->y_res /= PANGO_SCALE;
}

dynamic int text_get_res(text_t* text, uint64_t* x_res_ref, uint64_t* y_res_ref) {
	if (!x_res_ref) {
		LOG_ERROR("%p: X resolution reference argument is NULL", text)
	}

	if (!y_res_ref) {
		LOG_ERROR("%p: Y resolution reference argument is NULL", text)
	}

	gen_layout(text);

	*x_res_ref = text->x_res;
	*y_res_ref = text->y_res;

	return 0;
}

dynamic int text_pos_to_i(text_t* text, uint64_t x, uint64_t y) {
	gen_layout(text);

	/* TODO */

	return -1;
}

dynamic int text_i_to_pos(text_t* text, uint64_t i, uint64_t* x_ref, uint64_t* y_ref, uint64_t* width_ref, uint64_t* height_ref) {
	if (!x_ref) {
		LOG_ERROR("%p: X coordinate reference argument is NULL", text)
	}

	if (!y_ref) {
		LOG_ERROR("%p: Y coordinate reference argument is NULL", text)
	}

	if (!width_ref) {
		LOG_ERROR("%p: Width reference argument is NULL", text)
	}

	if (!height_ref) {
		LOG_ERROR("%p: Height reference argument is NULL", text)
	}

	gen_layout(text);

	/* TODO */

	return -1;
}

dynamic int draw_text(text_t* text, uint8_t** bmp_ref) {
	if (!bmp_ref) {
		LOG_ERROR("%p: Bitmap reference argument is NULL", text)
	}

	gen_layout(text);

	// create cairo surface & context

	cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, text->x_res, text->y_res);
	cairo_t* cairo = cairo_create(surface);

	// set the layout to our text's layout and also set the text colour

	pango_cairo_update_layout(cairo, text->layout);
	cairo_set_source_rgba(text->cairo, text->r, text->g, text->b, text->a);

	// draw the layout

	pango_cairo_show_layout(cairo, text->layout);
	cairo_surface_flush(surface);

	// copy data to bitmap
	// usually I wouldn't errorcheck 'malloc', but here there's a reasonable chance we may be out of memory and not instantly just die

	size_t bytes = cairo_image_surface_get_stride(surface) * text->y_res;
	uint8_t* bmp = malloc(bytes);

	if (!bmp) {
		LOG_FATAL("Out of memory (tried allocating %zu bytes)", bytes)
		return -1;
	}

	memcpy(bmp, cairo_image_surface_get_data(surface), bytes);
	*bmp_ref = bmp;

	// free everything

	cairo_destroy(cairo);
	cairo_surface_destroy(surface);

	return 0;
}
