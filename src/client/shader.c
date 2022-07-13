#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "client/client_config.h"
#include "client/opengl.h"
#include "client/shader.h"

static GLuint compile_shader(GLenum type, const char *path, const char *name, GLuint program, const char *def)
{
	char full_path[strlen(path) + 1 + strlen(name) + 1 + 4 + 1];
	sprintf(full_path, "%s/%s.glsl", path, name);

	FILE *file = fopen(full_path, "r");
	if (!file) {
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

	GLuint id = glCreateShader(type); GL_DEBUG

	const char *version = client_config.texture_batching
		? "#version 400 core\n"
		: "#version 330 core\n";

	const char *code_list[3] = {
		version,
		def,
		code,
	};

	int size_list[3] = {
		18,
		strlen(def),
		size,
	};

	glShaderSource(id, 3, code_list, size_list); GL_DEBUG

	glCompileShader(id); GL_DEBUG

	GLint success;
	glGetShaderiv(id, GL_COMPILE_STATUS, &success); GL_DEBUG
	if (!success) {
		char errbuf[BUFSIZ];
		glGetShaderInfoLog(id, BUFSIZ, NULL, errbuf); GL_DEBUG
		fprintf(stderr, "[error] failed to compile shader %s: %s", full_path, errbuf);
		glDeleteShader(id); GL_DEBUG
		return 0;
	}

	glAttachShader(program, id); GL_DEBUG

	return id;
}

GLuint shader_program_create(const char *path, const char *def)
{
	GLuint id = glCreateProgram(); GL_DEBUG

	if (!def)
		def = "";

	GLuint vert, frag;

	if (!(vert = compile_shader(GL_VERTEX_SHADER, path, "vertex", id, def))) {
		glDeleteProgram(id); GL_DEBUG
		abort();
	}

	if (!(frag = compile_shader(GL_FRAGMENT_SHADER, path, "fragment", id, def))) {
		glDeleteShader(vert); GL_DEBUG
		glDeleteProgram(id); GL_DEBUG
		abort();
	}

	glLinkProgram(id); GL_DEBUG
	glDeleteShader(vert); GL_DEBUG
	glDeleteShader(frag); GL_DEBUG

	GLint success;
	glGetProgramiv(id, GL_LINK_STATUS, &success); GL_DEBUG
	if (!success) {
		char errbuf[BUFSIZ];
		glGetProgramInfoLog(id, BUFSIZ, NULL, errbuf); GL_DEBUG
		fprintf(stderr, "[error] failed to link shader program %s: %s\n", path, errbuf);
		glDeleteProgram(id); GL_DEBUG
		abort();
	}

	return id;
}
