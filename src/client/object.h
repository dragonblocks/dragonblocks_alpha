#ifndef _OBJECT_H_
#define _OBJECT_H_

#include <stddef.h>
#include <stdbool.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <linmath.h/linmath.h>
#include "client/mesh.h"
#include "client/texture.h"
#include "client/vertex.h"
#include "array.h"
#include "types.h"

typedef struct {
	GLfloat x, y, z;
} __attribute__((packed)) Vertex3DPosition;

typedef struct {
	GLfloat s, t;
} __attribute__((packed)) Vertex3DTextureCoordinates;

typedef struct {
	GLfloat h, s, v;
} __attribute__((packed)) Vertex3DColor;

typedef struct
{
	Vertex3DPosition position;
	Vertex3DTextureCoordinates textureCoordinates;
	Vertex3DColor color;
} __attribute__((packed)) Vertex3D;

typedef struct
{
	GLuint texture;
	Array vertices;
} ObjectFace;

typedef struct
{
	v3f pos, rot, scale;
	f32 angle;
	bool remove;
	Mesh **meshes;
	size_t meshes_count;
	mat4x4 transform;
	bool visible;
	bool wireframe;
	ObjectFace *current_face;
	Array faces;
} Object;

Object *object_create();
void object_delete(Object *obj);
void object_set_texture(Object *obj, Texture *texture);
void object_add_vertex(Object *obj, Vertex3D *vertex);
bool object_add_to_scene(Object *obj);
void object_transform(Object *obj);
void object_render(Object *obj, GLint loc_model);

#endif