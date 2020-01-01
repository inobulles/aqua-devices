
#include <stdint.h>
#include <locale.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"

#define PRECISION 1000000

typedef struct {
	stbtt_fontinfo info;
} font_t;

typedef struct {
	uint8_t* pixels;
	
	uint64_t width, height;
	uint64_t x, y;
} surface_t;

uint64_t** kos_bda_pointer = (uint64_t**) 0;
uint64_t* video_height_pointer;

void handle(uint64_t** result_pointer_pointer, uint64_t* data) {
	uint64_t* kos_bda = *kos_bda_pointer;
	*result_pointer_pointer = &kos_bda[0];
	
	if (data[0] == 0x63) { // create
		void* raw_ttf_data = (void*) data[1];
		uint64_t __attribute__((unused)) raw_ttf_data_bytes = data[2];
		
		font_t* font = (font_t*) malloc(sizeof(font_t));
		
		if (!stbtt_InitFont(&font->info, (const uint8_t*) raw_ttf_data, stbtt_GetFontOffsetForIndex((const uint8_t*) raw_ttf_data, 0))) {
			kos_bda[0] = 0; // failure
			free(font);
			return;
		}
		
		kos_bda[0] = (uint64_t) font;
		
	} else if (data[0] == 0x72) { // remove
		font_t* font = (font_t*) data[1];
		free(font); // stb_trutype.h tells us, at line 721, that we don't need to do anything to free font->info
		
	} else if (data[0] == 0x74) { // text
		STBTT_FLATNESS = 0.2f; // set "quality" of the font (higher values are faster at the cost of lower quality) (note that this is something I added to lines 3532 and 3566, and does not come with stb_truetype by default)
		
		font_t* font = (font_t*) data[1];
		const char* string = (const char*) data[2];
		uint64_t bytes = strlen(string) + 1;
		
		uint64_t video_height = *video_height_pointer;
		int font_height = (int) ((float) data[3] / PRECISION * video_height / 2); // height in pixels
		
		// get metrics
		
		int __ascent, __descent, __line_gap;
		float scale = stbtt_ScaleForPixelHeight(&font->info, font_height);
		stbtt_GetFontVMetrics(&font->info, &__ascent, &__descent, &__line_gap);
		
		float ascent   = scale * __ascent;
		float descent  = scale * __descent;
		float line_gap = scale * __line_gap;
		
		float baseline = ascent;
		float line_spacing = ascent - descent + line_gap;
		
		// allocate bitmaps for each character
		
		uint64_t bitmap_count = 0;
		surface_t* bitmaps = (surface_t*) malloc((bitmap_count + 1) * sizeof(surface_t));
		
		float x = 2.0f, y = 2.0f;
		surface_t result_bitmap;
		memset(&result_bitmap, 0, sizeof(result_bitmap));
		
		font_t* buffer = (font_t*) malloc(sizeof(*font));
		memcpy(buffer, font, sizeof(*font));
		
		// create a bitmap for each character in the string
		
		for (uint64_t i = 0; i < bytes - 1; i++) { /// TODO UTF-8, wrapped text
			int current = string[i];
			
			if (current == '\n') { // move down on newline
				if (bitmap_count) {
					result_bitmap.width = fmax(result_bitmap.width, x + bitmaps[bitmap_count - 1].width);
				}
				
				y += line_spacing;
				x = 2.0f;
				
			} else { // create bitmap for any other character
				float shiftx = x - (float) floor(x);
				float shifty = y - (float) floor(y);
				
				int advance_width, left_side_bearing;
				stbtt_GetCodepointHMetrics(&buffer->info, current, &advance_width, &left_side_bearing);
				
				int x0, y0, x1, y1;
				stbtt_GetCodepointBitmapBoxSubpixel(&font->info, current, scale, scale, shiftx, shifty, &x0, &y0, &x1, &y1);
				
				bitmaps = (surface_t*) realloc(bitmaps, (bitmap_count + 1) * sizeof(surface_t));
				
				int width, height;
				int offsetx, offsety;
				
				bitmaps[bitmap_count].pixels = (uint8_t*) stbtt_GetCodepointBitmapSubpixel(&font->info, scale, scale, shiftx, shifty, current, &width, &height, &offsetx, &offsety);
				
				bitmaps[bitmap_count].width  = width;
				bitmaps[bitmap_count].height = height;
				
				bitmaps[bitmap_count].x = offsetx + x;
				bitmaps[bitmap_count].y = y + baseline + y0;
				
				bitmap_count++;
				x += scale * advance_width;
				
				int next = string[i + 1];
				if (next) {
					x += scale * stbtt_GetCodepointKernAdvance(&font->info, current, next);
				}
			}
		}
		
		// prepare result bitmap for drawing each individual character bitmap
		
		result_bitmap.width  = fmax(result_bitmap.width, x/* + bitmaps[bitmap_count - 1].width*/);
		result_bitmap.height = y + line_spacing;
		
		bytes = result_bitmap.width * result_bitmap.height * 4;
		result_bitmap.pixels = (uint8_t*) malloc(bytes);
		memset(result_bitmap.pixels, 0xFF, bytes); // set colour to white
		for (uint64_t i = 0; i < result_bitmap.width * result_bitmap.height; i++) result_bitmap.pixels[i * 4 + 3] = 0x00; // default to fully transparent
		
		// draw each character bitmap on big bitmap
		
		for (uint64_t i = 0; i < bitmap_count; i++) {
			for (uint64_t j = 0; j < bitmaps[i].width * bitmaps[i].height; j++) {
				if (bitmaps[i].width) {
					uint64_t k = (bitmaps[i].x + j % bitmaps[i].width + (bitmaps[i].y + j / bitmaps[i].width) * result_bitmap.width) * 4 + 3; // select alpha channel
					if (k < bytes) result_bitmap.pixels[k] += bitmaps[i].pixels[j];
				}
			}
			
			free(bitmaps[i].pixels);
		}
		
		free(bitmaps);
		
		// write everything to bda
		
		kos_bda[0] = (uint64_t) result_bitmap.pixels;
		kos_bda[1] = result_bitmap.width;
		kos_bda[2] = result_bitmap.height;
	}
}

