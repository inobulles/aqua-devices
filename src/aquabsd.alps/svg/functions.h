
dynamic int free_svg(svg_t* svg) {
	if (svg->handle) {
		g_object_unref(svg->handle);
	}

	free(svg);
	return 0;
}

static inline uint64_t __hash_str(const char* str) { // djb2 algorithm
	uint64_t hash = 5381;
	char c;

	while ((c = *str++)) {
		hash = ((hash << 5) + hash) + c; // hash * 33 + c
	}

	return hash;
}

dynamic svg_t* load_svg(const char* mem) {
	LOG_INFO("Load SVG (%p)", mem)

	gsize len = strlen(mem);

	GError* error = NULL;
	RsvgHandle* handle = rsvg_handle_new_from_data((const guint8*) mem, len, &error);

	if (error) {
		LOG_ERROR("Could not load SVG")
		return NULL;
	}

	// create SVG object

	svg_t* svg = calloc(1, sizeof *svg);

	svg->hash = __hash_str(mem);
	svg->handle = handle;

	LOG_VERBOSE("SVG hash is %lx", svg->hash)

#if defined(NEW_RSVG)
	rsvg_handle_get_intrinsic_size_in_pixels(svg->handle, &svg->width, &svg->height);
#else
	RsvgDimensionData dimensions;
	rsvg_handle_get_dimensions(svg->handle, &dimensions);

	svg->width  = dimensions.width;
	svg->height = dimensions.height;
#endif

	LOG_SUCCESS("Loaded SVG (%gx%g): %p", svg->width, svg->height, svg)

	return svg;
}

dynamic int draw_svg(svg_t* svg, uint64_t size, uint8_t** bitmap_ref, uint64_t* width_ref, uint64_t* height_ref) {
	if (!bitmap_ref) {
		LOG_ERROR("%p: Bitmap reference argument is NULL", svg)
	}

	if (!width_ref) {
		LOG_ERROR("%p: Width reference argument is NULL", svg)
	}

	if (!height_ref) {
		LOG_ERROR("%p: Height reference argument is NULL", svg)
	}

	LOG_VERBOSE("%p: Draw SVG", svg)

	int width  = size * svg->width / svg->height;
	int height = size;

	cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	cairo_t* cairo = cairo_create(surface);

	cairo_scale(cairo, (float) width / svg->width, (float) height / svg->height);

	// cairo_set_source_rgba(cairo, 0.0, 0.0, 0.0, 0.0); // note that cairo premultiplies our alpha
	// cairo_paint(cairo); // set background colour

#if defined(NEW_RSVG)
	RsvgRectangle viewport = { 50.0, 50.0, 50.0, 50.0 };
	rsvg_handle_render_document(svg->handle, cairo, &viewport, NULL);
#else
	rsvg_handle_render_cairo(svg->handle, cairo);
#endif

	cairo_surface_flush(surface);

	// copy data to bitmap

	LOG_VERBOSE("%p: Copy draw data to bitmap (%dx%d)", svg, width, height)

	unsigned bytes = cairo_image_surface_get_stride(surface) * height;
	uint8_t* bitmap = malloc(bytes);

	memcpy(bitmap, cairo_image_surface_get_data(surface), bytes);

	// set our references

	*width_ref  = width;
	*height_ref = height;

	*bitmap_ref = bitmap;

	// free everything

	cairo_surface_destroy(surface);
	cairo_destroy(cairo);

	return 0;
}
