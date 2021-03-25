#include <stdlib.h>
#include "mesh.h"

Mesh *mesh_create(GLfloat *vertices, GLsizei size)
{
	Mesh *mesh = malloc(sizeof(Mesh));
	mesh->pos = (v3f) {0.0f, 0.0f, 0.0f};
	mesh->rot = (v3f) {0.0f, 0.0f, 0.0f};
	mesh->scale = (v3f) {1.0f, 1.0f, 1.0f};
	mesh_transform(mesh);
	mesh->angle = 0.0f;
	mesh->VAO = 0;
	mesh->VBO = 0;
	mesh->remove = false;
	mesh->vertices = vertices;
	mesh->size = size;
	mesh->count = size / 6;
	return mesh;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wpedantic"

void mesh_transform(Mesh *mesh)
{
	mat4x4_translate(mesh->transform, mesh->pos.x, mesh->pos.y, mesh->pos.z);
	mat4x4_rotate(mesh->transform, mesh->transform, mesh->rot.x, mesh->rot.y, mesh->rot.z, mesh->angle);
	mat4x4_scale_aniso(mesh->transform, mesh->transform, mesh->scale.x, mesh->scale.y, mesh->scale.z);
}

#pragma GCC diagnostic pop

void mesh_delete(Mesh *mesh)
{
	if (mesh->vertices)
		free(mesh->vertices);
	if (mesh->VAO)
		glDeleteVertexArrays(1, &mesh->VAO);
	if (mesh->VBO)
		glDeleteBuffers(1, &mesh->VAO);
	free(mesh);
}

static void mesh_configure(Mesh *mesh)
{
	glGenVertexArrays(1, &mesh->VAO);
	glGenBuffers(1, &mesh->VBO);

	glBindVertexArray(mesh->VAO);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);

	glBufferData(GL_ARRAY_BUFFER, mesh->size, mesh->vertices, GL_STATIC_DRAW);

	GLsizei stride = 6 * sizeof(GLfloat);

	glVertexAttribPointer(0, 3, GL_FLOAT, false, stride, (GLvoid *)(0 * sizeof(GLfloat)));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, false, stride, (GLvoid *)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	free(mesh->vertices);
	mesh->vertices = NULL;
}

void mesh_render(Mesh *mesh, ShaderProgram *prog)
{
	if (mesh->vertices)
		mesh_configure(mesh);

	glUniformMatrix4fv(prog->loc_model, 1, GL_FALSE, mesh->transform[0]);

	glBindVertexArray(mesh->VAO);
	glDrawArrays(GL_TRIANGLES, 0, mesh->count);
	glBindVertexArray(0);
}
