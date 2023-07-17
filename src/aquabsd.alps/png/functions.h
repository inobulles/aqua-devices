
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>

#include <umber.h>
#define UMBER_COMPONENT "aquabsd.alps.png"

dynamic int free_png(png_t* png) {
	if (!png) {
		return -1;
	}

	if (png->bitmap) {
		free(png->bitmap);
	}

	if (png->png) {
		png_destroy_read_struct(&png->png, NULL, NULL);
	}

	free(png);
	return 0;
}

static inline png_t* __load_png(header, header_len) // pre-ANSI C go brrrr
	size_t header_len;
	char header[header_len];
{
	png_t* png = calloc(1, sizeof *png);

	// make sure file is PNG

	if (png_sig_cmp((png_const_bytep) header, 0, header_len)) {
		LOG_ERROR("File is not a PNG image")
		goto error; // file ain't PNG
	}

	// setup libpng

	png->png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png->png) {
		LOG_ERROR("png_create_read_struct: Allocation failed")
		goto error;
	}

	png->info = png_create_info_struct(png->png);

	if (!png->info) {
		LOG_ERROR("png_create_info_struct: Allocation failed")
		goto error;
	}

	if (setjmp(png_jmpbuf(png->png))) {
		LOG_ERROR("setjmp: %s", strerror(errno))
		goto error;
	}

	return png;

error:

	free_png(png);
	return NULL;
}

static inline void __read_info(png_t* png, size_t header_len) {
	png_set_sig_bytes(png->png, header_len);

	png_read_info(png->png, png->info);

	png->width = png_get_image_width(png->png, png->info);
	png->height = png_get_image_height(png->png, png->info);

	png->colour_type = png_get_color_type(png->png, png->info);
	png->bit_depth = png_get_bit_depth(png->png, png->info);

	png->number_of_passes = png_set_interlace_handling(png->png);
	png_read_update_info(png->png, png->info);
}

static void _read_data_from_input_stream(png_structp png, png_bytep ptr, png_size_t bytes) {
	// http://pulsarengine.com/2009/01/reading-png-images-from-memory/

	stream_t* stream = png_get_io_ptr(png);

	if (!stream) {
		LOG_WARN("Stream doesn't seem to exist")
		return;
	}

	memcpy(ptr, stream->ptr, bytes);
	stream->ptr += bytes;
}

static inline int __render_bitmap(png_t* png) {
	if (setjmp(png_jmpbuf(png->png))) {
		LOG_ERROR("setjmp: %s", strerror(errno))
		return -1;
	}

	png->row_bytes = png_get_rowbytes(png->png, png->info);
	png->bytes = png->row_bytes * png->height;

	png->bitmap = malloc(png->bytes);

	png_bytep* row_pointers = malloc(png->height * sizeof *row_pointers);

	for (size_t i = 0; i < png->height; i++) {
		row_pointers[i] = png->bitmap + png->row_bytes * i;
	}

	png_read_image(png->png, row_pointers);
	free(row_pointers); // TODO this is never freed when long jump occurs

	return 0;
}

dynamic png_t* load_png(void* ptr) {
	LOG_INFO("Load PNG (%p)", ptr)

	if (!ptr) {
		LOG_ERROR("Passed pointer is NULL")
		return NULL;
	}

	char header[8];
	memcpy(header, ptr, sizeof header);

	png_t* png = __load_png(header, sizeof header);

	if (!png) {
		return NULL;
	}

	png->stream.ptr = ptr + sizeof header;
	png_set_read_fn(png->png, &png->stream, _read_data_from_input_stream);

	__read_info(png, sizeof header);
	__render_bitmap(png);

	LOG_SUCCESS("Loaded PNG (%dx%dx%d): %p", png->width, png->height, png->bit_depth, png)

	return png;
}

dynamic int draw_png(png_t* png, uint8_t** bitmap_ref, uint64_t* bpp_ref, uint64_t* width_ref, uint64_t* height_ref) {
	// TODO maybe there's a better API to be made which isn't based off of aquabsd.alps.svg?
	//      after further reflection, it would probably be nice to have some kind of "meta-device" which opaquely handles all image types
	//      the only complication would be with SVG's, where we must explicitly specify the resolution at which to render the image
	//      in which case maybe SVG's should be the only "image format" to have its own separate device

	LOG_VERBOSE("%p: Draw PNG", png)

	// set references we already know about

	if (width_ref) {
		*width_ref = png->width;
	}

	if (height_ref) {
		*height_ref = png->height;
	}

	// read image data only if we haven't yet got a cached rendered bitmap
	// as a side note, what a weird API

	if (!png->bitmap && __render_bitmap(png) < 0) {
		return -1;
	}

	// set bitmap reference

	if (bitmap_ref) {
		*bitmap_ref = malloc(png->bytes);
		memcpy(*bitmap_ref, png->bitmap, png->bytes);
	}

	if (bpp_ref) {
		*bpp_ref = png->row_bytes / png->width * 8;
	}

	// TODO in theory, free everything
	// it's just that, from a *very* quick overview, it doesn't seem like libpng needs anything to be freed???
	// like afaict, as long as the file pointer is closed, everything is fine (cf. free_png), which is weird but ok
	// idk maybe i'm hella wrong it's late i don't wanna read into this

	return 0;
}
