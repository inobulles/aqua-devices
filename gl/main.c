
#include <stdint.h>
#include <stdlib.h>

#if KOS_PLATFORM == KOS_PLATFORM_DESKTOP
	#define GL_GLEXT_PROTOTYPES

	#include <GL/gl.h>
	#include <GL/glext.h>
#elif KOS_PLATFORM == KOS_PLATFORM_BROADCOM
	#include "GLES2/gl2.h"
#endif

uint64_t** kos_bda_pointer = (uint64_t**) 0;

void handle(uint64_t** result_pointer_pointer, uint64_t* data) {
	uint64_t* kos_bda = *kos_bda_pointer;
	*result_pointer_pointer = &kos_bda[0];
	
	if (data[0] == 0x74) { // bind_texture
		GLenum texture_slot = (GLenum) data[1];
		GLuint texture_id = (GLuint) data[2];
		
		glActiveTexture(GL_TEXTURE0 + texture_slot);
		glBindTexture(GL_TEXTURE_2D, texture_id);
		
	} else if (data[0] == 0x61) { // attribute
		uint64_t attribute_slot = data[1];
		uint64_t attribute_components = data[2];
		void* attribute_pointer = (void*) data[3];
		
		glEnableVertexAttribArray(attribute_slot);
		glVertexAttribPointer(attribute_slot, attribute_components, GL_FLOAT, GL_FALSE, attribute_components * sizeof(float), attribute_pointer);
		
	} else if (data[0] == 0x64) { // draw
		uint64_t index_count = data[1];
		void* index_pointer = (void*) data[2];
		
		uint16_t* index_pointer_16bit = (uint16_t*) malloc(index_count * sizeof(uint16_t));
		for (uint32_t i = 0; i < index_count; i++) index_pointer_16bit[i] = (uint16_t) ((uint32_t*) index_pointer)[i];
		
		glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_SHORT, index_pointer_16bit);
		free(index_pointer_16bit);
	}
}

