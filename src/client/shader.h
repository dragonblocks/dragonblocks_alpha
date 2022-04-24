#ifndef _SHADER_H_
#define _SHADER_H_

#include <GL/glew.h>
#include <GL/gl.h>
#include <stdbool.h>

GLuint shader_program_create(const char *path, const char *def);

#endif // _SHADER_H_
