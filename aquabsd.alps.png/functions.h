
#include <stdlib.h>
#include <stdint.h>

dynamic int free_png(png_t* png) {
	if (png->fp) {
		fclose(png->fp);
	}

	free(png);
	return 0;
}

dynamic png_t* load_png(const char* path) {
	png_t* png = calloc(1, sizeof *png);
	png->fp = fopen(path, "rb");

	if (!png->fp) {
		goto error;
	}

	char header[8];
	fread(header, 1, sizeof header, png->fp);

	if (png_sig_cmp((png_const_bytep) header, 0, sizeof header)) {
		goto error; // file ain't PNG
	}

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

	png_init_io(png->png, png->fp);
	png_set_sig_bytes(png->png, sizeof header);
	png_read_info(png->png, png->info);

	png->width = png_get_image_width(png->png, png->info);
	png->height = png_get_image_height(png->png, png->info);

	png->colour_type = png_get_color_type(png->png, png->info);
	png->bit_depth = png_get_bit_depth(png->png, png->info);

	png->number_of_passes = png_set_interlace_handling(png->png);
	png_read_update_info(png->png, png->info);

	return png;

error:

	if (png) free_png(png);
	return NULL;
}

dynamic int draw_png(png_t* png, uint8_t** bitmap_reference, uint64_t* bpp_reference, uint64_t* width_reference, uint64_t* height_reference) {
	// set references we already know about
	// TODO maybe there's a better API to be made which isn't based off of aquabsd.alps.svg?

	*width_reference  = png->width;
	*height_reference = png->height;

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

	*bitmap_reference = bitmap;
	*bpp_reference = row_bytes / png->width * 8;

	// TODO in theory, free everything
	// it's just that, from a *very* quick overview, it doesn't seem like libpng needs anything to be freed???
	// like afaict, as long as the file pointer is closed, everything is fine (cf. free_png), which is wierd but ok
	// idk maybe i'm hella wrong it's late i don't wanna read into this

	return 0;
}