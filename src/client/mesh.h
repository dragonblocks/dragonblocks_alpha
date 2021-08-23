#ifndef _MESH_H_
#define _MESH_H_

#include <GL/glew.h>
#include <GL/gl.h>
#include <stdbool.h>
#include "client/vertex.h"

typedef struct
{
	GLuint VAO, VBO;
	GLuint *textures;
	GLuint textures_count;
	bool free_textures;
	GLvoid *vertices;
	GLuint vertices_count;
	bool free_vertices;
	VertexLayout *layout;
} Mesh;

Mesh *mesh_create();
void mesh_delete(Mesh *mesh);
void mesh_configure(Mesh *mesh);
void mesh_render(Mesh *mesh);

#endif
