#include <stddef.h>
#include <stdlib.h>
#include "client/gl_debug.h"
#include "client/mesh.h"

// upload data to GPU (only done once)
void mesh_upload(Mesh *mesh)
{
	glGenVertexArrays(1, &mesh->vao); GL_DEBUG
	glGenBuffers(1, &mesh->vbo); GL_DEBUG

	glBindVertexArray(mesh->vao); GL_DEBUG
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo); GL_DEBUG

	glBufferData(GL_ARRAY_BUFFER, mesh->count * mesh->layout->size,
		mesh->data, GL_STATIC_DRAW); GL_DEBUG

	size_t offset = 0;
	for (GLuint i = 0; i < mesh->layout->count; i++) {
		VertexAttribute *attrib = &mesh->layout->attributes[i];

		glVertexAttribPointer(i, attrib->length, attrib->type, GL_FALSE,
			mesh->layout->size, (GLvoid *) offset); GL_DEBUG
		glEnableVertexAttribArray(i); GL_DEBUG

		offset += attrib->size;
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0); GL_DEBUG
	glBindVertexArray(0); GL_DEBUG

	if (mesh->free_data)
		free(mesh->data);

	mesh->data = NULL;
}

void mesh_render(Mesh *mesh)
{
	if (mesh->data)
		mesh_upload(mesh);

	// render
	glBindVertexArray(mesh->vao); GL_DEBUG
	glDrawArrays(GL_TRIANGLES, 0, mesh->count); GL_DEBUG
}

void mesh_destroy(Mesh *mesh)
{
	if (mesh->data && mesh->free_data)
		free(mesh->data);

	if (mesh->vao) {
		glDeleteVertexArrays(1, &mesh->vao); GL_DEBUG
	}

	if (mesh->vbo) {
		glDeleteBuffers(1, &mesh->vbo); GL_DEBUG
	}

	mesh->vao = mesh->vbo = 0;
}
