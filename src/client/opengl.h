#ifndef _OPENGL_H_
#define _OPENGL_H_

#ifdef ENABLE_GL_DEBUG
#define GL_DEBUG opengl_debug(__FILE__, __LINE__);
#else
#define GL_DEBUG
#endif

#include <GL/glew.h>
#include <GL/gl.h>

void opengl_debug(const char *file, int line);

#endif
