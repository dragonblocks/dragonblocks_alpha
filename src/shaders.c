#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "shaders.h"

static GLuint compile_and_attach_shader(GLenum type, const char *path, const char *name, GLuint program)
{
	char full_path[strlen(path) + 1 + strlen(name) + 1 + 4 + 1];
	sprintf(full_path, "%s/%s.glsl", path, name);

	FILE *file = fopen(full_path, "r");
	if (! file) {
		perror("fopen");
		return 0;
	}

	if (fseek(file, 0, SEEK_END) == -1) {
		perror("fseek");
		fclose(file);
		return 0;
	}

	long size = ftell(file);

	if (size == 1) {
		perror("ftell");
		fclose(file);
		return 0;
	}

	if (fseek(file, 0, SEEK_SET) == -1) {
		perror("fseek");
		fclose(file);
		return 0;
	}

	char code[size];

	if (fread(code, 1, size, file) != (size_t) size) {
		perror("fread");
		fclose(file);
		return 0;
	}

	fclose(file);

	GLuint id = glCreateShader(type);

	char const *codeptr = code;
	const int isize = (int) size;
	glShaderSource(id, 1, &codeptr, &isize);

	glCompileShader(id);

	GLint success;
	glGetShaderiv(id, GL_COMPILE_STATUS, &success);
	if (! success) {
		char errbuf[BUFSIZ];
		glGetShaderInfoLog(id, BUFSIZ, NULL, errbuf);
		fprintf(stderr, "Failed to compile %s shader: %s\n", name, errbuf);
		glDeleteShader(id);
		return 0;
	}

	glAttachShader(program, id);

	return id;
}


ShaderProgram *create_shader_program(const char *path)
{
	GLuint id = glCreateProgram();

	GLuint vert, frag;

	if (! (vert = compile_and_attach_shader(GL_VERTEX_SHADER, path, "vertex", id))) {
		glDeleteProgram(id);
		return NULL;
	}

	if (! (frag = compile_and_attach_shader(GL_FRAGMENT_SHADER, path, "fragment", id))) {
		glDeleteShader(vert);
		glDeleteProgram(id);
		return NULL;
	}

	glLinkProgram(id);
	glDeleteShader(vert);
	glDeleteShader(frag);

	GLint success;
	glGetProgramiv(id, GL_LINK_STATUS, &success);
	if (! success) {
		char errbuf[BUFSIZ];
		glGetProgramInfoLog(id, BUFSIZ, NULL, errbuf);
		fprintf(stderr, "Failed to link shader program: %s\n", errbuf);
		glDeleteProgram(id);
		return NULL;
	}

	ShaderProgram *prog = malloc(sizeof(ShaderProgram));
	prog->id = id;
	prog->loc_model = glGetUniformLocation(id, "model");
	prog->loc_view = glGetUniformLocation(id, "view");
	prog->loc_projection = glGetUniformLocation(id, "projection");

	return prog;
}

void delete_shader_program(ShaderProgram *prog)
{
	glDeleteProgram(prog->id);
	free(prog);
}

