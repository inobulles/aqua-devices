
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if KOS_PLATFORM == KOS_PLATFORM_DESKTOP
	#define GL_GLEXT_PROTOTYPES

	#include <GL/gl.h>
	#include <GL/glext.h>
#elif KOS_PLATFORM == KOS_PLATFORM_BROADCOM
	#include "GLES2/gl2.h"
#endif

#define PRECISION 1000000

typedef struct {
	float width, height;
	float red, green, blue, alpha;
	
	uint8_t has_texture;
	uint64_t texture;
	
	float scroll_x1, scroll_y1;
	float scroll_x2, scroll_y2;
	
	float coords [4 * 2];
	float colours[4 * 4];
} surface_t;

static const uint8_t indices[] = { 0, 1, 2, 0, 2, 3 };

static const float default_vertices[] = {
	-0.5,  0.5, 1.0,
	-0.5, -0.5, 1.0,
	 0.5, -0.5, 1.0,
	 0.5,  0.5, 1.0,
};

static const float default_coords[] = {
	0.0, 0.0,
	0.0, 1.0,
	1.0, 1.0,
	1.0, 0.0,
};

static void set_coords(surface_t* surface) {
	float width  = surface->scroll_x2 - surface->scroll_x1;
	float height = surface->scroll_y2 - surface->scroll_y1;
	
	for (int i = 0; i < 4; i++) {
		surface->coords[i * 2 + 0] = default_coords[i * 2 + 0] * width  + surface->scroll_x1;
		surface->coords[i * 2 + 1] = default_coords[i * 2 + 1] * height + surface->scroll_y1;
	}
}

static void set_colour(surface_t* surface) {
	for (int i = 0; i < 4; i++) {
		surface->colours[i * 4 + 0] = surface->red;
		surface->colours[i * 4 + 1] = surface->green;
		surface->colours[i * 4 + 2] = surface->blue;
		surface->colours[i * 4 + 3] = surface->alpha;
	}
}

#include "shader.h"
static GLint shader_program;

static const char* vertex_shader_code = R"shader-code(
attribute vec4 vertex_position;
attribute vec2 texture_coord;
attribute vec4 vertex_colour;

varying vec2 interpolated_texture_coord;
varying vec4 interpolated_vertex_colour;

void main(void) {
	interpolated_vertex_colour = vertex_colour;
	interpolated_texture_coord = texture_coord;
	
	gl_Position = vertex_position;
}
)shader-code";

static const char* fragment_shader_code = R"shader-code(
#if GL_ES
	precision mediump float;
#endif

varying vec2 interpolated_texture_coord;
varying vec4 interpolated_vertex_colour;

uniform sampler2D texture_sampler;
uniform int has_texture;

void main(void) {
	gl_FragColor = interpolated_vertex_colour;
	if (has_texture) gl_FragColor *= texture2D(texture_sampler, interpolated_texture_coord);
}
)shader-code";

static GLint has_texture_uniform_location = -1;
static GLint texture_sampler_uniform_location = -1;

void use_shader(GLint has_texture, GLint texture) {
	glUseProgram(shader_program);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	
	glUniform1i(has_texture_uniform_location, has_texture);
	glUniform1i(texture_sampler_uniform_location, 0);
}

void load(void) {
	create_shader_program(&shader_program, (char*) vertex_shader_code, (char*) fragment_shader_code);
	
	has_texture_uniform_location = glGetUniformLocation(shader_program, "has_texture");
	texture_sampler_uniform_location = glGetUniformLocation(shader_program, "texture_sampler");
}

void quit(void) {
	glDeleteProgram(shader_program);
}

static float offset;

void before_flip(void) {
	offset = 0.f;
}

uint64_t** kos_bda_pointer = (uint64_t**) 0;

void handle(uint64_t** result_pointer_pointer, uint64_t* data) {
	uint64_t* kos_bda = *kos_bda_pointer;
	*result_pointer_pointer = &kos_bda[0];
	
	if (data[0] == 0x63) { // create
		surface_t* surface = (surface_t*) malloc(sizeof(surface_t));
		memset(surface, 0, sizeof(*surface));
		
		surface->width  = 2.0;
		surface->height = 2.0;
		
		surface->scroll_x1 = 0.0;
		surface->scroll_y1 = 0.0;
		
		surface->scroll_x2 = 1.0;
		surface->scroll_y2 = 1.0;
		
		surface->red   = 1.0;
		surface->green = 1.0;
		surface->blue  = 1.0;
		surface->alpha = 1.0;
		
		surface->has_texture = 0;
		surface->texture = 0;
		
		set_coords(surface);
		set_colour(surface);
		
		kos_bda[0] = (uint64_t) surface;
		
	} else if (data[0] == 0x72) { // remove
		surface_t* surface = (surface_t*) data[1];
		free(surface);
		
	} else if (data[0] == 0x64) { // draw
		surface_t* surface = (surface_t*) data[1];
		
		float x = (float) (int64_t) data[2] / PRECISION;
		float y = (float) (int64_t) data[3] / PRECISION;
		
		offset += 0.01;
		float layer = -offset - data[4];
		
		use_shader(surface->has_texture, surface->texture);
		
		// draw surface
		
		float vertices[sizeof(default_vertices) / sizeof(*default_vertices)];
		for (int i = 0; i < 4; i++) {
			vertices[i * 3 + 2] = default_vertices[i * 3 + 2] * layer / 256;
			
			vertices[i * 3 + 0] = default_vertices[i * 3 + 0] * surface->width  + x;
			vertices[i * 3 + 1] = default_vertices[i * 3 + 1] * surface->height + y;
		}
		
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), vertices);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), surface->coords);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), surface->colours);
		
		glDrawElements(GL_TRIANGLES, sizeof(indices) / sizeof(*indices), GL_UNSIGNED_BYTE, indices);
		
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		
	} else if (data[0] == 0x74) { // texture
		surface_t* surface = (surface_t*) data[1];
		uint64_t texture_id = data[2];
		
		surface->texture = texture_id;
		surface->has_texture = 1;
		
	} else if (data[0] == 0x75) { // colour
		surface_t* surface = (surface_t*) data[1];
		
		int64_t red   = data[2];
		int64_t green = data[3];
		int64_t blue  = data[4];
		int64_t alpha = data[5];
		
		surface->red   = (float) red   / PRECISION;
		surface->green = (float) green / PRECISION;
		surface->blue  = (float) blue  / PRECISION;
		surface->alpha = (float) alpha / PRECISION;
		
		set_colour(surface);
		
	} else if (data[0] == 0x7A) { // size
		surface_t* surface = (surface_t*) data[1];
		
		int64_t width  = data[2];
		int64_t height = data[3];
		
		surface->width  = (float) width  / PRECISION;
		surface->height = (float) height / PRECISION;
		
	} else if (data[0] == 0x73) { // scroll
		surface_t* surface = (surface_t*) data[1];
		
		int64_t x1 = data[2];
		int64_t y1 = data[3];
		
		int64_t x2 = data[4];
		int64_t y2 = data[5];
		
		surface->scroll_x1 = (float) x1 / PRECISION / 2;
		surface->scroll_y1 = (float) y1 / PRECISION / 2;
		
		surface->scroll_x2 = (float) x2 / PRECISION / 2;
		surface->scroll_y2 = (float) y2 / PRECISION / 2;
		
		set_coords(surface);
	}
}

