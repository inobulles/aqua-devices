
// TODO make 'path' actually be the path somehow

dynamic font_t* load_font(const char* path) {
	PangoFontDescription* font_description = pango_font_description_from_string(path);

	font_t* font = calloc(1, sizeof *font);
	font->font_description = font_description;

	return font;
}

dynamic int free_font(font_t* font) {
	pango_font_description_free(font->font_description);
	return 0;
}

// yes, I very much know this is incredibly stupid
// again though, it's not my fault if I'm forced to use crappy API's

// TODO include a 'size' argument

dynamic int draw_font(font_t* font, const char* string, float red, float green, float blue, float alpha, uint64_t wrap_width, uint64_t wrap_height, uint8_t** bitmap_reference, uint64_t* width_reference, uint64_t* height_reference) {
	// create dummy cairo surface and pango layout for getting text dimensions
	
	cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
	cairo_t* cairo = cairo_create(surface);

	PangoLayout* layout = pango_cairo_create_layout(cairo);
	pango_layout_set_font_description(layout, font->font_description);

	if (wrap_width  > 0x2000) wrap_width  = 0x2000;
	if (wrap_height > 0x2000) wrap_height = 0x2000;

	if (wrap_width ) pango_layout_set_width (layout, wrap_width  * PANGO_SCALE);
	else             pango_layout_set_width (layout, -1);

	if (wrap_height) pango_layout_set_height(layout, wrap_height * PANGO_SCALE);
	else             pango_layout_set_height(layout, -1);
	
	pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);

	// actually get text dimensions

	pango_layout_set_markup(layout, string, -1); // we use 'pango_layout_set_markup' instead of 'pango_layout_set_text' here for obvious reasons
	
	int width, height;

	pango_cairo_update_layout(cairo, layout);
	pango_layout_get_size(layout, &width, &height);

	width  /= PANGO_SCALE;
	height /= PANGO_SCALE;

	// free that dummy cairo surface

	cairo_surface_destroy(surface);

	g_object_unref(layout);
	cairo_destroy(cairo);

	// create new cairo surface and pango layout, this time for actually drawing

	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	cairo = cairo_create(surface);

	cairo_set_source_rgba(cairo, 0.0, 0.0, 0.0, 0.0); // note that cairo premultiplies our alpha
	cairo_paint(cairo); // set background colour

	layout = pango_cairo_create_layout(cairo);
	pango_layout_set_font_description(layout, font->font_description);

	if (wrap_width ) pango_layout_set_width (layout, wrap_width  * PANGO_SCALE);
	else             pango_layout_set_width (layout, -1);

	if (wrap_height) pango_layout_set_height(layout, wrap_height * PANGO_SCALE);
	else             pango_layout_set_height(layout, -1);
	
	pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);

	// actually (x3 lol) draw text

	pango_layout_set_markup(layout, string, -1); // we use 'pango_layout_set_markup' instead of 'pango_layout_set_text' here for obvious reasons
	cairo_set_source_rgba(cairo, red, green, blue, alpha); // font colour

	pango_cairo_update_layout(cairo, layout);

	pango_cairo_show_layout(cairo, layout);
	cairo_surface_flush(surface);

	// copy data to bitmap

	unsigned bytes = cairo_image_surface_get_stride(surface) * height;
	uint8_t* bitmap = (uint8_t*) malloc(bytes);

	memcpy(bitmap, cairo_image_surface_get_data(surface), bytes);

	// set our references

	*width_reference  = width;
	*height_reference = height;
	
	*bitmap_reference = bitmap;

	// free everything

	cairo_surface_destroy(surface);

	g_object_unref(layout);
	cairo_destroy(cairo);

	return 0;
}

