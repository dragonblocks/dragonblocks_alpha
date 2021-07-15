#ifndef _SHADER_H_
#define _SHADER_H_

#include <stdbool.h>
#include <GL/glew.h>
#include <GL/gl.h>

bool shader_program_create(const char *path, GLuint *idptr);

#endif
