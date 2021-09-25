#include <stdlib.h>
#include <stddef.h>
#include "client/mesh.h"

Mesh *mesh_create()
{
	Mesh *mesh = malloc(sizeof(Mesh));
	mesh->VAO = mesh->VBO = 0;
	mesh->free_textures = false;
	mesh->free_vertices = false;

	return mesh;
}

void mesh_delete(Mesh *mesh)
{
	if (mesh->textures && mesh->free_textures)
		free(mesh->textures);

	if (mesh->vertices && mesh->free_vertices)
		free(mesh->vertices);

	if (mesh->VAO)
		glDeleteVertexArrays(1, &mesh->VAO);

	if (mesh->VBO)
		glDeleteBuffers(1, &mesh->VAO);

	free(mesh);
}

void mesh_configure(Mesh *mesh)
{
	glGenVertexArrays(1, &mesh->VAO);
	glGenBuffers(1, &mesh->VBO);

	glBindVertexArray(mesh->VAO);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);

	glBufferData(GL_ARRAY_BUFFER, mesh->vertices_count * mesh->layout->size, mesh->vertices, GL_STATIC_DRAW);

	vertex_layout_configure(mesh->layout);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	if (mesh->free_vertices)
		free(mesh->vertices);

	mesh->vertices = NULL;
}

void mesh_render(Mesh *mesh)
{
	if (mesh->vertices)
		mesh_configure(mesh);

	for (GLuint i = 0; i < mesh->textures_count; i++)
		glBindTextureUnit(i, mesh->textures[i]);

	glBindVertexArray(mesh->VAO);
	glDrawArrays(GL_TRIANGLES, 0, mesh->vertices_count);
}
