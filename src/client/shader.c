#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "client/gl_debug.h"
#include "client/shader.h"

static GLuint compile_shader(GLenum type, const char *path, const char *name, GLuint program, const char *defs)
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

	// Minimum OpenGL version is 4.2.0 (idk some shader feature from that version is required)
	const char *version = "#version 420 core\n"; // 420 blaze it

	const char *code_list[3] = {
		version,
		defs,
		code,
	};

	int size_list[3] = {
		18,
		strlen(defs),
		size,
	};

	glShaderSource(id, 3, code_list, size_list); GL_DEBUG

	glCompileShader(id); GL_DEBUG

	GLint success;
	glGetShaderiv(id, GL_COMPILE_STATUS, &success); GL_DEBUG
	if (!success) {
		char errbuf[BUFSIZ];
		glGetShaderInfoLog(id, BUFSIZ, NULL, errbuf); GL_DEBUG
		fprintf(stderr, "[error] failed to compile %s shader: %s", name, errbuf);
		glDeleteShader(id); GL_DEBUG
		return 0;
	}

	glAttachShader(program, id); GL_DEBUG

	return id;
}

bool shader_program_create(const char *path, GLuint *idptr, const char *defs)
{
	GLuint id = glCreateProgram(); GL_DEBUG

	if (!defs)
		defs = "";

	GLuint vert, frag;

	if (!(vert = compile_shader(GL_VERTEX_SHADER, path, "vertex", id, defs))) {
		glDeleteProgram(id); GL_DEBUG
		return false;
	}

	if (!(frag = compile_shader(GL_FRAGMENT_SHADER, path, "fragment", id, defs))) {
		glDeleteShader(vert); GL_DEBUG
		glDeleteProgram(id); GL_DEBUG
		return false;
	}

	glLinkProgram(id); GL_DEBUG
	glDeleteShader(vert); GL_DEBUG
	glDeleteShader(frag); GL_DEBUG

	GLint success;
	glGetProgramiv(id, GL_LINK_STATUS, &success); GL_DEBUG
	if (!success) {
		char errbuf[BUFSIZ];
		glGetProgramInfoLog(id, BUFSIZ, NULL, errbuf); GL_DEBUG
		fprintf(stderr, "[error] failed to link shader program: %s\n", errbuf);
		glDeleteProgram(id); GL_DEBUG
		return false;
	}

	*idptr = id;
	return true;
}
