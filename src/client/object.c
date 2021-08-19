#include <stdlib.h>
#include "client/object.h"
#include "client/scene.h"

static VertexAttribute vertex_attributes[3] = {
	// position
	{
		.type = GL_FLOAT,
		.length = 3,
		.size = sizeof(Vertex3DPosition),
	},
	// textureCoordinates
	{
		.type = GL_FLOAT,
		.length = 2,
		.size = sizeof(Vertex3DTextureCoordinates),

	},
	// color
	{
		.type = GL_FLOAT,
		.length = 3,
		.size = sizeof(Vertex3DColor),
	},
};

static VertexLayout vertex_layout = {
	.attributes = vertex_attributes,
	.count = 3,
	.size = sizeof(Vertex3D),
};

Object *object_create()
{
	Object *obj = malloc(sizeof(Object));
	obj->pos = (v3f32) {0.0f, 0.0f, 0.0f};
	obj->rot = (v3f32) {0.0f, 0.0f, 0.0f};
	obj->scale = (v3f32) {1.0f, 1.0f, 1.0f};
	obj->angle = 0.0f;
	obj->remove = false;
	obj->meshes = NULL;
	obj->meshes_count = 0;
	obj->visible = true;
	obj->wireframe = false;
	obj->current_face = NULL;
	obj->faces = array_create(sizeof(ObjectFace));

	return obj;
}

void object_delete(Object *obj)
{
	for (size_t i = 0; i < obj->meshes_count; i++)
		mesh_delete(obj->meshes[i]);

	free(obj);
}

void object_set_texture(Object *obj, Texture *texture)
{
	if (obj->current_face && obj->current_face->texture == texture->id)
		return;

	ObjectFace face = {
		.texture = texture->id,
		.vertices = array_create(sizeof(Vertex3D)),
	};

	array_append(&obj->faces, &face);
	obj->current_face = &((ObjectFace *) obj->faces.ptr)[obj->faces.siz - 1];
}

void object_add_vertex(Object *obj, Vertex3D *vertex)
{
	array_append(&obj->current_face->vertices, vertex);
}

static int qsort_compare_faces(const void *f1, const void *f2)
{
	return ((ObjectFace *) f1)->texture - ((ObjectFace *) f2)->texture;
}

static void add_mesh(Array *meshes, Array *vertices, GLuint texture)
{
	if (vertices->siz > 0) {
		Mesh *mesh = mesh_create();
		mesh->vertices = vertices->ptr;
		mesh->vertices_count = vertices->siz;
		mesh->free_vertices = true;
		mesh->texture = texture;
		mesh->layout = &vertex_layout;

		array_append(meshes, &mesh);
	}

	*vertices = array_create(sizeof(Vertex3D));
}

bool object_add_to_scene(Object *obj)
{
	if (obj->faces.siz == 0)
		return false;

	object_transform(obj);

	qsort(obj->faces.ptr, obj->faces.siz, sizeof(ObjectFace), &qsort_compare_faces);

	Array meshes = array_create(sizeof(Mesh *));
	Array vertices = array_create(sizeof(Vertex3D));
	GLuint texture = 0;

	for (size_t f = 0; f < obj->faces.siz; f++) {
		ObjectFace *face = &((ObjectFace *) obj->faces.ptr)[f];

		if (face->texture != texture) {
			add_mesh(&meshes, &vertices, texture);
			texture = face->texture;
		}

		for (size_t v = 0; v < face->vertices.siz; v++)
			array_append(&vertices, &((Vertex3D *) face->vertices.ptr)[v]);
		free(face->vertices.ptr);
	}
	add_mesh(&meshes, &vertices, texture);
	free(obj->faces.ptr);

	array_copy(&meshes, (void *) &obj->meshes, &obj->meshes_count);
	free(meshes.ptr);

	scene_add_object(obj);

	return true;
}

void object_transform(Object *obj)
{
	mat4x4_translate(obj->transform, obj->pos.x, obj->pos.y, obj->pos.z);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
	mat4x4_rotate(obj->transform, obj->transform, obj->rot.x, obj->rot.y, obj->rot.z, obj->angle);
	mat4x4_scale_aniso(obj->transform, obj->transform, obj->scale.x, obj->scale.y, obj->scale.z);
#pragma GCC diagnostic pop
}

void object_render(Object *obj, GLint loc_model)
{
	if (! obj->visible)
		return;

	glUniformMatrix4fv(loc_model, 1, GL_FALSE, obj->transform[0]);

	if (obj->wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	for (size_t i = 0; i < obj->meshes_count; i++)
		mesh_render(obj->meshes[i]);

	if (obj->wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
