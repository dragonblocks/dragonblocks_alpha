#ifndef _SHADERS_H_
#define _SHADERS_H_

#include <GL/glew.h>
#include <GL/gl.h>

typedef struct
{
	GLuint id;
	GLint loc_model;
	GLint loc_view;
	GLint loc_projection;
} ShaderProgram;

ShaderProgram *create_shader_program(const char *path);
void delete_shader_program(ShaderProgram *prog);

#endif
