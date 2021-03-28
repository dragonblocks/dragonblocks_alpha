#ifndef _MESH_H_
#define _MESH_H_

#include <GL/glew.h>
#include <GL/gl.h>
#include <linmath.h/linmath.h>
#include <stdbool.h>
#include "shaders.h"
#include "types.h"

typedef struct
{
	v3f pos, rot, scale;
	float angle;
	mat4x4 transform;
	GLuint VAO, VBO;
	bool remove;
	GLfloat *vertices;
	GLsizei count;
} Mesh;

typedef struct
{
	GLfloat x, y, z;
	GLfloat r, g, b;
} __attribute__((packed, aligned(4))) Vertex;

Mesh *mesh_create(Vertex *vertices, GLsizei count);
void mesh_delete(Mesh *mesh);
void mesh_transform(Mesh *mesh);
void mesh_render(Mesh *mesh, ShaderProgram *prog);

#endif
