
#include <stdlib.h>
#include <stdint.h>

dynamic int free_png(png_t* png) {
	if (!png) {
		return -1;
	}

	if (png->png) {
		png_destroy_read_struct(&png->png, NULL, NULL);
	}

	if (png->fp) {
		fclose(png->fp);
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
		goto error; // file ain't PNG
	}

	// setup libpng

	png->png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png->png) {
		goto error;
	}

	png->info = png_create_info_struct(png->png);

	if (!png->info) {
		goto error;
	}

	if (setjmp(png_jmpbuf(png->png))) {
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

dynamic png_t* load_png(const char* path) {
	FILE* fp = fopen(path, "rb");

	if (fp) {
		return NULL;
	}

	char header[8];
	fread(header, 1, sizeof header, fp);

	png_t* png = __load_png(header, sizeof header);

	if (!png) {
		fclose(fp);
		return NULL;
	}

	png->fp = fp;
	png_init_io(png->png, png->fp);

	__read_info(png, sizeof header);

	return png;
}

static void _read_data_from_input_stream(png_structp png, png_bytep ptr, png_size_t bytes) {
	// http://pulsarengine.com/2009/01/reading-png-images-from-memory/

	stream_t* stream = png_get_io_ptr(png);

	if (!stream) {
		return;
	}

	memcpy(ptr, stream->ptr, bytes);
	stream->ptr += bytes;
}

dynamic png_t* load_png_ptr(void* ptr) {
	if (!ptr) {
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

	return png;
}

dynamic int draw_png(png_t* png, uint8_t** bitmap_reference, uint64_t* bpp_reference, uint64_t* width_reference, uint64_t* height_reference) {
	// set references we already know about
	// TODO maybe there's a better API to be made which isn't based off of aquabsd.alps.svg?
	//      after further reflection, it would probably be nice to have some kind of "meta-device" which opaquely handles all image types
	//      the only complication would be with SVG's, where we must explicitly specify the resolution at which to render the image

	if (width_reference) {
		*width_reference = png->width;
	}

	if (height_reference) {
		*height_reference = png->height;
	}

	// read image data
	// as a side note, what a wierd API

	if (setjmp(png_jmpbuf(png->png))) {
		return -1;
	}

	unsigned row_bytes = png_get_rowbytes(png->png, png->info);
	uint8_t* bitmap = malloc(row_bytes * png->height);

	png_bytep* row_pointers = malloc(png->height * sizeof *row_pointers);

	for (int i = 0; i < png->height; i++) {
		row_pointers[i] = bitmap + row_bytes * i;
	}

	png_read_image(png->png, row_pointers);
	free(row_pointers);

	// set bitmap reference

	if (bitmap_reference) {
		*bitmap_reference = bitmap;
	}

	else {
		free(bitmap);
	}

	if (bpp_reference) {
		*bpp_reference = row_bytes / png->width * 8;
	}

	// TODO in theory, free everything
	// it's just that, from a *very* quick overview, it doesn't seem like libpng needs anything to be freed???
	// like afaict, as long as the file pointer is closed, everything is fine (cf. free_png), which is weird but ok
	// idk maybe i'm hella wrong it's late i don't wanna read into this

	return 0;
}
