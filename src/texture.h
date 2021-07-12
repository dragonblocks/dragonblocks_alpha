#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include <GL/glew.h>
#include <GL/gl.h>

typedef struct
{
	GLuint id;
	int width, height;
} Texture;

Texture *get_texture(char *path);

#endif
