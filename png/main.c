
#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION

#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

uint64_t** kos_bda_pointer = (uint64_t**) 0;

void handle(uint64_t** result_pointer_pointer, uint64_t* data) {
	uint64_t* kos_bda = *kos_bda_pointer;
	*result_pointer_pointer = &kos_bda[0];
	
	void* image_data = (void*) data[1];
	uint64_t bytes = data[2];
	
	uint64_t bit_depth;
	uint64_t width;
	uint64_t height;
	
	if (data[0] == 0x64) { // decode
		stbi_set_flip_vertically_on_load(0);
		image_data = stbi_load_from_memory((const stbi_uc*) image_data, bytes, (int*) &width, (int*) &height, (int*) &bit_depth, 0);
		bit_depth *= 8;
		
	} else if (data[0] == 0x65) { // encode
		/// TODO
	}
	
	kos_bda[0] = (uint64_t) image_data;
	kos_bda[1] = bit_depth;
	kos_bda[2] = width;
	kos_bda[3] = height;
}

