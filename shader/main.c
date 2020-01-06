
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#if KOS_PLATFORM == KOS_PLATFORM_DESKTOP
	#define GL_GLEXT_PROTOTYPES

	#include <GL/gl.h>
	#include <GL/glext.h>
#elif KOS_PLATFORM == KOS_PLATFORM_BROADCOM
	#include "GLES2/gl2.h"
#endif

#define PRECISION 1000000

#define UNIFORM_TYPE_FLOAT    0x00
#define UNIFORM_TYPE_INT      0x01

#define UNIFORM_SIZE_VALUE    0x00
#define UNIFORM_SIZE_VECTOR_2 0x10
#define UNIFORM_SIZE_VECTOR_3 0x20
#define UNIFORM_SIZE_VECTOR_4 0x30
#define UNIFORM_SIZE_MATRIX   0x40

static int create_shader(GLuint shader, char* code) {
	glShaderSource (shader, 1, (const GLchar**) &code, NULL);
	glCompileShader(shader);
	
	GLint compile_status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
	
	if (compile_status == GL_FALSE) {
		int log_length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
		
		if (log_length > 0) {
			char* error_message = (char*) malloc(log_length);
			glGetShaderInfoLog(shader, log_length, NULL, error_message);
			
			printf("ERROR Failed to compile shader (%s)\n", error_message);
			free(error_message);
			
		} else {
			printf("ERROR Failed to compile shader for some unknown reason\n");
		}
		
		return 1;
	}
	
	return 0;
}

static int create_shader_program(GLuint* program, char* vertex_code, char* fragment_code) {
	GLuint   vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	
	if (create_shader(vertex_shader,     vertex_code)) return 1;
	if (create_shader(fragment_shader, fragment_code)) return 1;
	
	*program = glCreateProgram();
	
	glAttachShader(*program, vertex_shader);
	glAttachShader(*program, fragment_shader);
	
	glBindAttribLocation(*program, 0, "vertex_position");
	glBindAttribLocation(*program, 1, "texture_coord");
	glBindAttribLocation(*program, 2, "vertex_colour");
	glBindAttribLocation(*program, 3, "vertex_normal");
	
	glLinkProgram(*program);
	
	GLint link_status;
	glGetProgramiv(*program, GL_LINK_STATUS, &link_status);
	
	if (link_status == GL_FALSE) {
		int log_length;
		glGetProgramiv(*program, GL_INFO_LOG_LENGTH, &log_length);
		
		if (log_length > 0) {
			char* error_message = (char*) malloc((unsigned) (log_length + 1));
			glGetProgramInfoLog(*program, log_length, NULL, error_message);
			
			printf("ERROR Failed to link program (%s)\n", error_message);
			free(error_message);
			
		} else {
			printf("ERROR Failed to link program for some unkown reason\n");
		}
		
		return 1;
	}
	
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	
	return 0;
}

uint64_t** kos_bda_pointer = (uint64_t**) 0;

static GLint int_buffer[4];
static GLfloat float_buffer[4];

void handle(uint64_t** result_pointer_pointer, uint64_t* data) {
	uint64_t* kos_bda = *kos_bda_pointer;
	*result_pointer_pointer = &kos_bda[0];
	
	if (data[0] == 0x63) { // create
		GLuint* program_pointer = (GLuint*) data[1];
		
		const char* vert_shader_code = (const char*) data[2];
		const char* frag_shader_code = (const char*) data[3];
		
		kos_bda[0] = create_shader_program(program_pointer, (char*) vert_shader_code, (char*) frag_shader_code);
		
	} else if (data[0] == 0x72) { // remove
		GLuint* program_pointer = (GLuint*) data[1];
		glDeleteProgram(*program_pointer);
		
	} else if (data[0] == 0x75) { // use
		GLuint* program_pointer = (GLuint*) data[1];
		glUseProgram(*program_pointer);
		
	} else if (data[0] == 0x66) { // find
		GLuint* program_pointer = (GLuint*) data[1];
		const char* name = (const char*) data[2];
		
		kos_bda[0] = glGetUniformLocation(*program_pointer, name);
		
	} else if (data[0] == 0x6C) { // uniform
		uint64_t uniform_location = data[1];
		uint64_t uniform_type = data[2];
		void* __data = (void*) data[3];
		
		uint64_t type = uniform_type & 0x0F;
		uint64_t size = uniform_type & 0xF0;
		
		if (size == UNIFORM_SIZE_MATRIX) {
			glUniformMatrix4fv(uniform_location, 1, GL_FALSE, __data);
			
		} else {
			int count = size + 1; // get the element count
			
			for (int i = 0; i < count; i++) {
				if (type == UNIFORM_TYPE_FLOAT) float_buffer[i] = (GLfloat) ((int64_t*) __data)[i] / PRECISION;
				else if (type == UNIFORM_TYPE_INT) int_buffer[i] = (GLint) ((int64_t*) __data)[i];
			}
			
			if (type == UNIFORM_TYPE_FLOAT) {
				if      (count == 1) glUniform1fv(uniform_location, 1, float_buffer);
				else if (count == 2) glUniform2fv(uniform_location, 1, float_buffer);
				else if (count == 3) glUniform3fv(uniform_location, 1, float_buffer);
				else if (count == 4) glUniform4fv(uniform_location, 1, float_buffer);
				
			} else if (type == UNIFORM_TYPE_INT) {
				if      (count == 1) glUniform1iv(uniform_location, 1, int_buffer);
				else if (count == 2) glUniform2iv(uniform_location, 1, int_buffer);
				else if (count == 3) glUniform3iv(uniform_location, 1, int_buffer);
				else if (count == 4) glUniform4iv(uniform_location, 1, int_buffer);
			}
		}
	}
}

