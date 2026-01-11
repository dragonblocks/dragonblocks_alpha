#ifndef _TEXTURE_H_
#define _TEXTURE_H_

#include <stdbool.h>
#include "client/opengl.h"

typedef struct {
	GLuint txo;
	int width, height, channels;
} Texture;

typedef struct {
	GLfloat tex_coord_x, tex_coord_y;
	GLfloat tex_coord_w, tex_coord_h;
	unsigned int width, height;
} TextureSlice;

typedef struct {
	Texture texture;
	size_t mipmap_levels;
	unsigned char **data;
	struct {
		unsigned int x, y, row_height;
	} alloc;
} TextureAtlas;

Texture *texture_load(char *path, bool mipmap);
Texture *texture_load_cubemap(char *path, bool linear_filter);
void texture_upload(Texture *texture, unsigned char *data, GLenum format, bool mipmap);
void texture_destroy(Texture *texture);

TextureAtlas texture_atlas_create(int width, int height, int channels, size_t mipmap_levels);
TextureSlice texture_atlas_add(TextureAtlas *atlas, char *path, bool x4);
Texture texture_atlas_upload(TextureAtlas *atlas);

#endif // _TEXTURE_H_
