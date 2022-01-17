#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include <GL/glew.h>
#include <GL/gl.h>

typedef struct
{
	GLuint id;
	int width, height;
} Texture;

Texture *texture_create(unsigned char *data, int width, int height, GLenum format, bool mipmap);
GLuint texture_create_cubemap(char *path);
void texture_delete(Texture *texture);
Texture *texture_load(char *path, bool mipmap);

#endif
