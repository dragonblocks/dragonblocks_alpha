#ifndef _FONT_H_
#define _FONT_H_

#include <stdbool.h>
#include <stddef.h>
#include "client/mesh.h"

typedef struct
{
	Mesh **meshes;
	size_t meshes_count;
} Font;

bool font_init();
void font_deinit();
Font *font_create(const char *text);
void font_delete(Font *fnt);
void font_render(Font *fnt);

#endif
