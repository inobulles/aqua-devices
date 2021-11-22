
#include <stdlib.h>
#include <stdint.h>

dynamic int free_svg(svg_t* svg) {
	if (svg->handle) {
		g_object_unref(svg->handle);
	}

	free(svg);
	return 0;
}

dynamic svg_t* load_svg(const char* path) {
	GError* error = NULL;
	RsvgHandle* handle = rsvg_handle_new_from_file(path, &error);

	if (error) {
		return (svg_t*) 0;
	}

	svg_t* svg = calloc(1, sizeof *svg);
	svg->handle = handle;

#if defined(NEW_RSVG)
	rsvg_handle_get_intrinsic_size_in_pixels(svg->handle, &svg->width, &svg->height);
#else
	RsvgDimensionData dimensions;
	rsvg_handle_get_dimensions(svg->handle, &dimensions);

	svg->width  = dimensions.width;
	svg->height = dimensions.height;
#endif

	return svg;

error:

	if (svg) free_svg(svg);
	return (svg_t*) 0;
}

dynamic int draw_svg(svg_t* svg, uint64_t size, uint8_t** bitmap_reference, uint64_t* width_reference, uint64_t* height_reference) {
	int width = size * svg->width / svg->height;
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

	unsigned bytes = cairo_image_surface_get_stride(surface) * height;
	uint8_t* bitmap = (uint8_t*) malloc(bytes);

	memcpy(bitmap, cairo_image_surface_get_data(surface), bytes);

	// set our references

	*width_reference  = width;
	*height_reference = height;
	
	*bitmap_reference = bitmap;

	// free everything

	cairo_surface_destroy(surface);
	cairo_destroy(cairo);

	return 0;
}
