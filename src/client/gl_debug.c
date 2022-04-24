#include <GL/glew.h>
#include <GL/gl.h>
#include <stdio.h>
#include "client/gl_debug.h"

// put this into seperate function to make it easy to trace stack using external debugger
static void gl_error(GLenum err, const char *file, int line)
{
	switch (err) {
		case GL_INVALID_ENUM:                  fprintf(stderr, "[warning] OpenGL error: INVALID_ENUM %s:%d\n", file, line); break;
		case GL_INVALID_VALUE:                 fprintf(stderr, "[warning] OpenGL error: INVALID_VALUE %s:%d\n", file, line); break;
		case GL_INVALID_OPERATION:             fprintf(stderr, "[warning] OpenGL error: INVALID_OPERATION %s:%d\n", file, line); break;
		case GL_STACK_OVERFLOW:                fprintf(stderr, "[warning] OpenGL error: STACK_OVERFLOW %s:%d\n", file, line); break;
		case GL_STACK_UNDERFLOW:               fprintf(stderr, "[warning] OpenGL error: STACK_UNDERFLOW %s:%d\n", file, line); break;
		case GL_OUT_OF_MEMORY:                 fprintf(stderr, "[warning] OpenGL error: OUT_OF_MEMORY %s:%d\n", file, line); break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: fprintf(stderr, "[warning] OpenGL error: INVALID_FRAMEBUFFER_OPERATION %s:%d\n", file, line); break;
		default: break;
	}
}
void gl_debug(const char *file, int line)
{
	GLenum err = glGetError();

	if (err != GL_NO_ERROR)
		gl_error(err, file, line);
}
