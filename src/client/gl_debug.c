#include <GL/glew.h>
#include <GL/gl.h>
#include <stdio.h>
#include "client/gl_debug.h"

// put this into seperate function to make it easy to trace stack using external debugger
static void gl_error(GLenum err, const char *file, int line)
{
	switch (err) {
		case GL_INVALID_ENUM:                  printf("INVALID_ENUM %s:%d\n", file, line); break;
		case GL_INVALID_VALUE:                 printf("INVALID_VALUE %s:%d\n", file, line); break;
		case GL_INVALID_OPERATION:             printf("INVALID_OPERATION %s:%d\n", file, line); break;
		case GL_STACK_OVERFLOW:                printf("STACK_OVERFLOW %s:%d\n", file, line); break;
		case GL_STACK_UNDERFLOW:               printf("STACK_UNDERFLOW %s:%d\n", file, line); break;
		case GL_OUT_OF_MEMORY:                 printf("OUT_OF_MEMORY %s:%d\n", file, line); break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: printf("INVALID_FRAMEBUFFER_OPERATION %s:%d\n", file, line); break;
		default: break;
	}
}
void gl_debug(const char *file, int line)
{
	GLenum err = glGetError();

	if (err != GL_NO_ERROR)
		gl_error(err, file, line);
}
