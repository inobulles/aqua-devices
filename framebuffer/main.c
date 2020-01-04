
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
	
	if (data[0] == 0x63) { // create
		GLuint colour_texture_id = (GLuint) data[1];
		GLuint  depth_texture_id = (GLuint) data[2];
		
		GLuint prev_framebuffer_id;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_framebuffer_id);
		
		GLuint framebuffer_id = 0;
		glGenFramebuffers(1, &framebuffer_id);
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);
		
		if (colour_texture_id) glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colour_texture_id, 0);
		if  (depth_texture_id) glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,   depth_texture_id, 0);
		
		kos_bda[0] = framebuffer_id;
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			kos_bda[0] = 0; // failure
		}
		
		glBindFramebuffer(GL_FRAMEBUFFER, prev_framebuffer_id);
		
	} else if (data[0] == 0x72) { // remove
		GLuint framebuffer_id = (GLuint) data[1];
		glDeleteFramebuffers(1, &framebuffer_id);
		
	} else if (data[0] == 0x62) { // bind
		GLuint framebuffer_id = (GLuint) data[1];
		
		int64_t x = data[2];
		int64_t y = data[3];
		
		uint64_t width  = data[4];
		uint64_t height = data[5];
		
		glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);
		glViewport((GLint) x, (GLint) y, (GLuint) width, (GLuint) height);
	}
}

