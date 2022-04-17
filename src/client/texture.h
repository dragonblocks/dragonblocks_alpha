#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include <GL/glew.h>
#include <GL/gl.h>

typedef struct {
	GLuint txo;
	int width, height, channels;
} Texture;

Texture *texture_load(char *path, bool mipmap);
Texture *texture_load_cubemap(char *path, bool linear_filter);
void texture_upload(Texture *texture, unsigned char *data, GLenum format, bool mipmap);
void texture_destroy(Texture *texture);

#endif // _TEXTURE_H_
