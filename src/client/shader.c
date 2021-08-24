#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "client/shader.h"

static GLuint compile_and_attach_shader(GLenum type, const char *path, const char *name, GLuint program, const char *definitions)
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

	const char *version = "#version 460 core\n";

	const char *code_list[3] = {
		version,
		definitions,
		code,
	};

	int size_list[3] = {
		18,
		strlen(definitions),
		size,
	};

	glShaderSource(id, 3, code_list, size_list);

	glCompileShader(id);

	GLint success;
	glGetShaderiv(id, GL_COMPILE_STATUS, &success);
	if (! success) {
		char errbuf[BUFSIZ];
		glGetShaderInfoLog(id, BUFSIZ, NULL, errbuf);
		fprintf(stderr, "Failed to compile %s shader: %s", name, errbuf);
		glDeleteShader(id);
		return 0;
	}

	glAttachShader(program, id);

	return id;
}

bool shader_program_create(const char *path, GLuint *idptr, const char *definitions)
{
	GLuint id = glCreateProgram();

	if (! definitions)
		definitions = "";

	GLuint vert, frag;

	if (! (vert = compile_and_attach_shader(GL_VERTEX_SHADER, path, "vertex", id, definitions))) {
		glDeleteProgram(id);
		return false;
	}

	if (! (frag = compile_and_attach_shader(GL_FRAGMENT_SHADER, path, "fragment", id, definitions))) {
		glDeleteShader(vert);
		glDeleteProgram(id);
		return false;
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
		return false;
	}

	*idptr = id;
	return true;
}
