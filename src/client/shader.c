#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "client/client_config.h"
#include "client/opengl.h"
#include "client/shader.h"
#include "common/fs.h"

static GLuint compile_shader(GLenum type, const char *path, const char *name, GLuint program, const char *def)
{
	char full_path[strlen(path) + 1 + strlen(name) + 1 + 4 + 1];
	sprintf(full_path, "%s/%s.glsl", path, name);

	char *code = read_file(full_path);
	if (!code)
		return 0;

	GLuint id = glCreateShader(type); GL_DEBUG

	const char *code_list[3] = {
		"#version 330 core\n",
		def,
		code,
	};

	int size_list[3] = {
		18,
		strlen(def),
		strlen(code),
	};

	glShaderSource(id, 3, code_list, size_list); GL_DEBUG
	glCompileShader(id); GL_DEBUG
	free(code);

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
