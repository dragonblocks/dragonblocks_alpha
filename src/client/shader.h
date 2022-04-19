#ifndef _SHADER_H_
#define _SHADER_H_

#include <GL/glew.h>
#include <GL/gl.h>
#include <stdbool.h>

bool shader_program_create(const char *path, GLuint *idptr, const char *defs);

#endif // _SHADER_H_
