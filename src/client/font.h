#ifndef _FONT_H_
#define _FONT_H_

#include <GL/glew.h>
#include <GL/gl.h>
#include <stdbool.h>
#include <stddef.h>
#include "client/mesh.h"
#include "types.h"

typedef struct {
	v2f32 size;
	size_t count;
	Mesh *meshes;
	GLuint *textures;
} Font;

bool font_init();
void font_deinit();
Font *font_create(const char *text);
void font_delete(Font *font);
void font_render(Font *font);

#endif // _FONT_H_
