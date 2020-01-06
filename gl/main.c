
#include <stdint.h>

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
		
	} else if (data[0] == 0x64) { // draw
		uint64_t index_count = data[1];
		void* index_pointer = (void*) data[2];
		
		float* vertex_pointer = (float*) data[3];
		float* texture_coords_pointer = (float*) data[4];
		float* colour_pointer = (float*) data[5];
		float* normal_pointer = (float*) data[6];
		
		if (vertex_pointer)         glEnableVertexAttribArray(0);
		if (texture_coords_pointer) glEnableVertexAttribArray(1);
		if (colour_pointer)         glEnableVertexAttribArray(2);
		if (normal_pointer)         glEnableVertexAttribArray(3);
		
		if (vertex_pointer)         glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), vertex_pointer);
		if (texture_coords_pointer) glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), texture_coords_pointer);
		if (colour_pointer)         glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), colour_pointer);
		if (normal_pointer)         glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), normal_pointer);
		
		glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_SHORT, (uint16_t*) index_pointer);
		
		if (vertex_pointer)         glDisableVertexAttribArray(0);
		if (texture_coords_pointer) glDisableVertexAttribArray(1);
		if (colour_pointer)         glDisableVertexAttribArray(2);
		if (normal_pointer)         glDisableVertexAttribArray(3);
	}
}

