#include <assert.h>
#include <stdlib.h>
#include <stddef.h>
#include "mesh.h"
#include "scene.h"

VertexBuffer vertexbuffer_create()
{
	return (VertexBuffer) {
		.faces = array_create(sizeof(Face)),
	};
}

void vertexbuffer_set_texture(VertexBuffer *buffer, Texture *texture)
{
	if (buffer->current && buffer->current->texture == texture->id)
		return;
	Face face = {
		.texture = texture->id,
		.vertices = array_create(sizeof(Vertex)),
	};
	array_append(&buffer->faces, &face);
	buffer->current = &((Face *) buffer->faces.ptr)[buffer->faces.siz - 1];
}

void vertexbuffer_add_vertex(VertexBuffer *buffer, Vertex *vertex)
{
	array_append(&buffer->current->vertices, vertex);
}

static int qsort_compare_faces(const void *f1, const void *f2)
{
	return ((Face *) f1)->texture - ((Face *) f2)->texture;
}

static void add_mesh(Array *meshes, Array *vertices, GLuint texture)
{
	if (vertices->siz > 0) {
		Mesh *mesh = malloc(sizeof(Mesh));
		mesh->VAO = mesh->VBO = 0;
		// the reason the vertices are not copied and then free'd like anything else is that the vertices will be deleted after the first render anyway
		mesh->vertices = vertices->ptr;
		mesh->vertices_count = vertices->siz;
		mesh->texture = texture;

		array_append(meshes, &mesh);
	}

	*vertices = array_create(sizeof(Vertex));
}

MeshObject *meshobject_create(VertexBuffer buffer, struct Scene *scene, v3f pos)
{
	if (buffer.faces.siz == 0)
		return NULL;

	MeshObject *obj = malloc(sizeof(MeshObject));
	obj->remove = false;

	obj->pos = pos;
	obj->rot = (v3f) {0.0f, 0.0f, 0.0f};
	obj->scale = (v3f) {1.0f, 1.0f, 1.0f};
	obj->angle = 0.0f;
	obj->visible = true;
	obj->wireframe = false;
	meshobject_transform(obj);

	qsort(buffer.faces.ptr, buffer.faces.siz, sizeof(Face), &qsort_compare_faces);

	Array meshes = array_create(sizeof(Mesh *));
	Array vertices = array_create(sizeof(Vertex));
	GLuint texture = 0;

	for (size_t f = 0; f < buffer.faces.siz; f++) {
		Face *face = &((Face *) buffer.faces.ptr)[f];

		if (face->texture != texture) {
			add_mesh(&meshes, &vertices, texture);
			texture = face->texture;
		}

		for (size_t v = 0; v < face->vertices.siz; v++)
			array_append(&vertices, &((Vertex *) face->vertices.ptr)[v]);
		free(face->vertices.ptr);
	}
	add_mesh(&meshes, &vertices, texture);
	free(buffer.faces.ptr);

	array_copy(&meshes, (void *) &obj->meshes, &obj->meshes_count);
	free(meshes.ptr);

	if (scene)
		scene_add_object(scene, obj);

	return obj;
}


void meshobject_transform(MeshObject *obj)
{
	mat4x4_translate(obj->transform, obj->pos.x, obj->pos.y, obj->pos.z);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
	mat4x4_rotate(obj->transform, obj->transform, obj->rot.x, obj->rot.y, obj->rot.z, obj->angle);
	mat4x4_scale_aniso(obj->transform, obj->transform, obj->scale.x, obj->scale.y, obj->scale.z);
#pragma GCC diagnostic pop
}


void meshobject_delete(MeshObject *obj)
{
	for (size_t i = 0; i < obj->meshes_count; i++) {
		Mesh *mesh = obj->meshes[i];

		if (mesh->vertices)
			free(mesh->vertices);

		if (mesh->VAO)
			glDeleteVertexArrays(1, &mesh->VAO);

		if (mesh->VBO)
			glDeleteBuffers(1, &mesh->VAO);

		free(mesh);
	}

	free(obj);
}

static void mesh_configure(Mesh *mesh)
{
	glGenVertexArrays(1, &mesh->VAO);
	glGenBuffers(1, &mesh->VBO);

	glBindVertexArray(mesh->VAO);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);

	glBufferData(GL_ARRAY_BUFFER, mesh->vertices_count * sizeof(Vertex), mesh->vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(Vertex), (GLvoid *) offsetof(Vertex, x));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(Vertex), (GLvoid *) offsetof(Vertex, s));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 3, GL_FLOAT, false, sizeof(Vertex), (GLvoid *) offsetof(Vertex, r));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	free(mesh->vertices);
	mesh->vertices = NULL;
}

void meshobject_render(MeshObject *obj, ShaderProgram *prog)
{
	if (! obj->visible)
		return;

	glUniformMatrix4fv(prog->loc_model, 1, GL_FALSE, obj->transform[0]);

	glActiveTexture(GL_TEXTURE0);

	if (obj->wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	for (size_t i = 0; i < obj->meshes_count; i++) {
		Mesh *mesh = obj->meshes[i];

		if (mesh->vertices)
			mesh_configure(mesh);

		glBindTexture(GL_TEXTURE_2D, mesh->texture);
		glBindVertexArray(mesh->VAO);

		glDrawArrays(GL_TRIANGLES, 0, mesh->vertices_count);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);

	if (obj->wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
