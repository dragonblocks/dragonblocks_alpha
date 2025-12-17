#ifndef DRAGONBLOCKS_GL_H_
	#define DRAGONBLOCKS_GL_H_

	#ifdef __APPLE__
		#include <OpenGL/gl.h>
	#else
		#include <GL/gl.h>
	#endif

	static inline void bind_texture_2d(GLuint unit, GLuint texture)
	{
		#ifdef __APPLE__
			glActiveTexture(GL_TEXTURE0 + unit);
			glBindTexture(GL_TEXTURE_2D, texture);
		#else
			glBindTextureUnit(unit, texture);
		#endif
	}

	static inline void bind_texture_cubemap(GLuint unit, GLuint texture)
	{
		#ifdef __APPLE__
			glActiveTexture(GL_TEXTURE0 + unit);
			glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
		#else
			glBindTextureUnit(unit, texture);
		#endif
	}
#endif
