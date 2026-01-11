#ifndef _MESH_H_
#define _MESH_H_

#include <stdbool.h>
#include "client/opengl.h"
#include "types.h"

typedef struct {
	GLenum type;
	GLsizei length;
	GLsizei size;
} VertexAttribute;

typedef struct {
	VertexAttribute *attributes;
	GLuint count;
	GLsizei size;
} VertexLayout;

typedef struct {
	VertexLayout *layout;
	GLuint vao, vbo;
	GLvoid *data;
	GLuint count;
	bool free_data;
} Mesh;

void mesh_load(Mesh *mesh, const char *path, aabb3s32 *extents);
void mesh_upload(Mesh *mesh);
void mesh_render(Mesh *mesh);
void mesh_destroy(Mesh *mesh);

#endif // _MESH_H_
