#ifndef _SHADERS_H_
#define _SHADERS_H_

typedef struct
{
	GLuint id;
	GLint loc_model;
	GLint loc_view;
	GLint loc_proj;
} ShaderProgram;

ShaderProgram *create_shader_program(const char *path);	// ToDo
void delete_shader_program(ShaderProgram *);

#endif
