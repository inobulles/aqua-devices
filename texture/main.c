
#include <stdint.h>
#include <stdlib.h>

#if KOS_PLATFORM == KOS_PLATFORM_DESKTOP
	#define GL_GLEXT_PROTOTYPES

	#include <GL/gl.h>
	#include <GL/glext.h>
#elif KOS_PLATFORM == KOS_PLATFORM_BROADCOM
	#include "GLES2/gl2.h"
#endif

#define FILTER_MAG_NONE             0x01
#define FILTER_MAG_BILINEAR         0x02

#define FILTER_MIN_NONE             0x10
#define FILTER_MIN_MIPMAPS          0x20
#define FILTER_MIN_BILINEAR         0x30
#define FILTER_MIN_BILINEAR_MIPMAPS 0x40
#define FILTER_MIN_TRILINEAR        0x50

#define TYPE_COLOUR 0x00
#define TYPE_DEPTH  0x01

static uint64_t filtering_mode;

void load(void) {
	filtering_mode = FILTER_MAG_BILINEAR | FILTER_MIN_TRILINEAR;
}

uint64_t** kos_bda_pointer = (uint64_t**) 0;

void handle(uint64_t** result_pointer_pointer, uint64_t* data) {
	uint64_t* kos_bda = *kos_bda_pointer;
	*result_pointer_pointer = &kos_bda[0];
	
	if (data[0] == 0x63) { // create
		void* _bitmap = (void*) data[1];
		
		uint64_t type = data[2];
		uint64_t bit_depth = data[3];
		
		uint64_t width  = data[4];
		uint64_t height = data[5];
		
		void* bitmap = _bitmap;
		//~ if (bit_depth > 32) { // assuming > 32 BPP's are unsupported by the HW ...
			//~ bit_depth /= 2;
			//~ uint64_t data_bytes = width * height * (bit_depth / 8);
			//~ bitmap = (uint64_t*) malloc(data_bytes);
			
			//~ for (uint64_t i = 0; i < data_bytes; i++) {
				//~ ((uint8_t*) bitmap)[i] = ((uint8_t*) _bitmap)[i << 1];
			//~ }
		//~ }
		
		GLenum format = GL_RGBA;
		GLint internal_format = GL_RGBA8;
		
		if (type == TYPE_COLOUR) {
			if (bit_depth == 8) {
				format = GL_RED;
				internal_format = GL_R8;
				
			} else if (bit_depth == 16) {
				format = GL_RG;
				internal_format = GL_RG8;
				
			} else if (bit_depth == 24) {
				format = GL_RGB;
				internal_format = GL_RGB;
				
			} else if (bit_depth == 32) {
				format = GL_RGBA;
				internal_format = GL_RGBA;
				
			} else if (bit_depth == 48) {
				format = GL_RGB;
				internal_format = GL_RGB16;
				
			} else if (bit_depth == 64) {
				format = GL_RGBA;
				internal_format = GL_RGBA16;
			}
			
		} else if (type == TYPE_DEPTH) {
			if (bit_depth == 16) {
				format = GL_DEPTH_COMPONENT;
				internal_format = GL_DEPTH_COMPONENT16;
				
			} else if (bit_depth == 24) {
				format = GL_DEPTH_COMPONENT;
				internal_format = GL_DEPTH_COMPONENT24;
				
			} else if (bit_depth == 32) {
				format = GL_DEPTH_COMPONENT;
				internal_format = GL_DEPTH_COMPONENT32;
			}
		}
		
		GLuint texture_id;
		glGenTextures(1, (GLuint*) &texture_id);
		
		glBindTexture(GL_TEXTURE_2D, (GLuint) texture_id);
		glTexImage2D(GL_TEXTURE_2D, 0, internal_format, (GLuint) width, (GLuint) height, 0, format, GL_UNSIGNED_BYTE, bitmap);
		
		if (bit_depth > 32) {
			free(bitmap);
		}
		
		uint64_t mag_filtering_mode = filtering_mode & 0x0F;
		uint64_t min_filtering_mode = filtering_mode & 0xF0;
		
		if      (mag_filtering_mode == FILTER_MAG_NONE            ) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		else if (mag_filtering_mode == FILTER_MAG_BILINEAR        ) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
		if      (min_filtering_mode == FILTER_MIN_NONE            ) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		else if (min_filtering_mode == FILTER_MIN_MIPMAPS         ) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		else if (min_filtering_mode == FILTER_MIN_BILINEAR        ) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		else if (min_filtering_mode == FILTER_MIN_BILINEAR_MIPMAPS) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		else if (min_filtering_mode == FILTER_MIN_TRILINEAR       ) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		
		if (min_filtering_mode == FILTER_MIN_MIPMAPS || min_filtering_mode == FILTER_MIN_BILINEAR_MIPMAPS || min_filtering_mode == FILTER_MIN_TRILINEAR) { // which filtering modes require mipmaps?
			glGenerateMipmap(GL_TEXTURE_2D);
		}
		
		// make sure textures are set to mirror
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		
		kos_bda[0] = texture_id;
		
	} else if (data[0] == 0x72) { // remove
		GLuint texture_id = (GLuint) data[1];
		glDeleteTextures(1, &texture_id);
		
	} else if (data[0] == 0x66) { // filter
		filtering_mode = data[1];
	}
}

