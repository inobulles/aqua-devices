
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

static int create_shader_program(GLuint* program, char** attribute_list, uint32_t attribute_count, char* vertex_code, char* fragment_code) {
	GLuint   vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	
	if (create_shader(vertex_shader,     vertex_code)) return 1;
	if (create_shader(fragment_shader, fragment_code)) return 1;
	
	*program = glCreateProgram();
	
	glAttachShader(*program, vertex_shader);
	glAttachShader(*program, fragment_shader);
	
	for (uint32_t i = 0; i < attribute_count; i++) {
		glBindAttribLocation(*program, i, attribute_list[i]);
	}
	
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

static void add_string(char** string_pointer, char* data) {
	uint32_t string_bytes = strlen(*string_pointer);
	uint32_t data_bytes = strlen(data);
	uint32_t bytes = string_bytes + data_bytes + 1;
	
	*string_pointer = (char*) realloc(*string_pointer, bytes);
	memcpy(*string_pointer + string_bytes, data, data_bytes + 1);
}

#define MAX_KEYWORD_BYTES 256
#define IS_SEPERATOR(x) (x=='!'||x=='%'||x=='&'||x=='('||x==')'||x=='*'||x=='+'||x==','||x=='-'||x=='.'||x=='/'||x==':'||x=='<'||x=='>'||x=='='||x=='?'||x=='['||x==']'||x=='^'||x=='{'||x=='}'||x=='|'||x=='~')

static char* asl_to_glsl(char** attribute_list, uint32_t* attribute_count, uint8_t fragment, const char* code) { // really basic ASL parser
	char* head = (char*) malloc(1);
	char* body = (char*) malloc(1);
	
	*head = 0;
	*body = 0;
	
	add_string(&head, "#pragma optimize(on)\n#if GLES\nprecision mediump float;\n#endif\n");
	add_string(&body, "void main(void){");
	
	char** section = &body;
	
	char keyword[MAX_KEYWORD_BYTES] = {0};
	uint32_t keyword_index = 0;
	
	uint8_t comment = 0;
	char* next_statement = (char*) 0;
	uint8_t next_attribute = 0;
	
	uint32_t bytes = strlen(code);
	for (uint32_t i = 0; i < bytes; i++) {
		char current = code[i];
		uint8_t seperator = IS_SEPERATOR(current);
		
		if (comment) {
			if (current == '\n') comment = 0;
			
		} else if (current == '/' && code[i + 1] == '/') { // comment
			comment = 1;
			
		} else if (\
			current == ' ' || current == '\t' || current == '\n' || current == ';' /* whitespaces */ \
			|| seperator) {
			
			// head stuff
			
			if (strcmp(keyword, "interp") == 0) { section = &head; add_string(section, "varying"); }
			else if (strcmp(keyword, "inform") == 0) { section = &head; add_string(section, "uniform"); }
			
			else if (strcmp(keyword, "attrib") == 0) {
				section = &head;
				add_string(section, "attribute");
				next_attribute = 1;
			}
			
			// type stuff
			
			else if (strcmp(keyword, "frozen") == 0) add_string(section, "const");
			else if (strcmp(keyword, "tex2") == 0) add_string(section, "sampler2D");
			else if (strcmp(keyword, "flt") == 0) add_string(section, "float");
			
			// function stuff
			
			else if (strcmp(keyword, "sample") == 0) add_string(section, "texture2D");
			
			// input stuff
			
			else if (strcmp(keyword, "input_coords") == 0) add_string(section, "gl_FragCoord");
			
			// amber stuff
			
			else if (strcmp(keyword, "loop") == 0) add_string(section, "while(1)");
			else if (strcmp(keyword, "or") == 0) add_string(section, "||");
			else if (strcmp(keyword, "and") == 0) add_string(section, "&&");
			else if (strcmp(keyword, "not") == 0) add_string(section, "!");
			else if (strcmp(keyword, "nrm") == 0) add_string(section, "!!");
			
			// other stuff
			
			else if (strcmp(keyword, "return") == 0) {
				next_statement = "return;";
				
				if (fragment) add_string(section, "gl_FragColor=");
				else add_string(section, "gl_Position=");
				
			} else {
				if (next_attribute && next_attribute++ == 2) {
					memcpy(attribute_list[(*attribute_count)++], keyword, sizeof(keyword));
					next_attribute = 0;
				}
				
				add_string(section, keyword);
			}
			
			if (current == ';') { // last statement finished
				add_string(section, ";");
				if (next_statement) add_string(section, next_statement);
				section = &body;
				
			} else if (seperator) {
				char temp[2] = "#";
				temp[0] = current;
				add_string(section, temp);
				
			} else {
				add_string(section, " ");
			}
			
			keyword_index = 0;
			memset(keyword, 0, sizeof(keyword));
			
		} else {
			keyword[keyword_index++] = current;
		}
	}
	
	add_string(&body, "}");
	
	uint32_t head_bytes = strlen(head);
	uint32_t body_bytes = strlen(body);
	
	char* result = (char*) malloc(head_bytes + body_bytes + 1);
	
	memcpy(result, head, head_bytes);
	memcpy(result + head_bytes, body, body_bytes + 1); // copy over the null terminator too
	
	free(head);
	free(body);
	
	return result;
}

uint64_t** kos_bda_pointer = (uint64_t**) 0;

static GLint int_buffer[4];
static GLfloat float_buffer[4];

void handle(uint64_t** result_pointer_pointer, uint64_t* data) {
	uint64_t* kos_bda = *kos_bda_pointer;
	*result_pointer_pointer = &kos_bda[0];
	
	if (data[0] == 0x63) { // create
		GLuint* program_pointer = (GLuint*) data[1];
		
		#define USE_GLSL 0
		
		#if USE_GLSL
			char* vert_shader_code = (const char*) data[2];
			char* frag_shader_code = (const char*) data[3];
			
			char* attribute_list[4] = {"vertex_position", "texture_coord", "vertex_colour", "vertex_normal"};
			uint32_t attribute_count = 4;
		#else
			const char* asl_vert_shader_code = (const char*) data[2];
			const char* asl_frag_shader_code = (const char*) data[3];
			
			#define MAX_ATTRIBUTE_COUNT 16
			char** attribute_list = (char**) malloc(sizeof(char*) * MAX_ATTRIBUTE_COUNT);
			for (int i = 0; i < MAX_ATTRIBUTE_COUNT; i++) attribute_list[i] = (char*) malloc(MAX_KEYWORD_BYTES);
			uint32_t attribute_count = 0;
			
			char* vert_shader_code = asl_to_glsl((char**) attribute_list, &attribute_count, 0, asl_vert_shader_code);
			char* frag_shader_code = asl_to_glsl((char**) attribute_list, &attribute_count, 1, asl_frag_shader_code);
		#endif
		
		kos_bda[0] = create_shader_program(program_pointer, attribute_list, attribute_count, vert_shader_code, frag_shader_code);
		
		#if !USE_GLSL
			for (int i = 0; i < MAX_ATTRIBUTE_COUNT; i++) {
				free(attribute_list[i]);
			}
			
			free(vert_shader_code);
			free(frag_shader_code);
		#endif
		
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
			int count = (size >> 4) + 1; // get the element count
			
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

