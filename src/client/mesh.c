#include <stddef.h>
#include <stdlib.h>
#include "client/mesh.h"

// upload data to GPU (only done once)
void mesh_upload(Mesh *mesh)
{
	glGenVertexArrays(1, &mesh->vao);
	glGenBuffers(1, &mesh->vbo);

	glBindVertexArray(mesh->vao);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);

	glBufferData(GL_ARRAY_BUFFER, mesh->count * mesh->layout->size,
		mesh->data, GL_STATIC_DRAW);

	size_t offset = 0;
	for (GLuint i = 0; i < mesh->layout->count; i++) {
		VertexAttribute *attrib = &mesh->layout->attributes[i];

		glVertexAttribPointer(i, attrib->length, attrib->type, GL_FALSE,
			mesh->layout->size, (GLvoid *) offset);
		glEnableVertexAttribArray(i);

		offset += attrib->size;
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	if (mesh->free_data)
		free(mesh->data);

	mesh->data = NULL;
}

void mesh_render(Mesh *mesh)
{
	if (mesh->data)
		mesh_upload(mesh);

	// render
	glBindVertexArray(mesh->vao);
	glDrawArrays(GL_TRIANGLES, 0, mesh->count);
}

void mesh_destroy(Mesh *mesh)
{
	if (mesh->data && mesh->free_data)
		free(mesh->data);

	if (mesh->vao)
		glDeleteVertexArrays(1, &mesh->vao);

	if (mesh->vbo)
		glDeleteBuffers(1, &mesh->vbo);

	mesh->vao = mesh->vbo = 0;
}
