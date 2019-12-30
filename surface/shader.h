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
