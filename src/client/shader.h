#ifndef _SHADER_H_
#define _SHADER_H_

#include <stdbool.h>
#include "client/opengl.h"

GLuint shader_program_create(const char *path, const char *def);

#endif // _SHADER_H_
