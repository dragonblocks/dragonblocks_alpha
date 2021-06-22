#ifndef _MESH_H_
#define _MESH_H_

#include <GL/glew.h>
#include <GL/gl.h>
#include <linmath.h/linmath.h>
#include <stdbool.h>
#include "array.h"
#include "shaders.h"
#include "types.h"

typedef struct
{
	GLfloat x, y, z;
	GLfloat s, t;
} __attribute__((packed)) Vertex;

typedef struct
{
	GLuint texture;
	Array vertices;
} Face;

typedef struct
{
	Face *current;
	Array faces;
} VertexBuffer;

typedef struct
{
	GLuint VAO, VBO;
	GLuint texture;
	Vertex *vertices;
	GLuint vertices_count;
} Mesh;

typedef struct
{
	v3f pos, rot, scale;
	float angle;
	mat4x4 transform;
	bool remove;
	Mesh **meshes;
	size_t meshes_count;
} MeshObject;

struct Scene;

VertexBuffer vertexbuffer_create();
void vertexbuffer_set_texture(VertexBuffer *buffer, GLuint texture);
void vertexbuffer_add_vertex(VertexBuffer *buffer, Vertex *vertex);

MeshObject *meshobject_create(VertexBuffer buffer, struct Scene *scene, v3f pos);
void meshobject_delete(MeshObject *obj);
void meshobject_transform(MeshObject *obj);
void meshobject_render(MeshObject *obj, ShaderProgram *prog);

#endif
