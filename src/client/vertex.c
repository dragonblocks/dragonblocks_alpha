#include "client/vertex.h"

void vertex_layout_configure(VertexLayout *layout)
{
	size_t offset = 0;

	for (GLuint i = 0; i < layout->count; i++) {
		VertexAttribute *attrib = &layout->attributes[i];

		glVertexAttribPointer(i, attrib->length, attrib->type, GL_FALSE, layout->size, (GLvoid *) offset);
		glEnableVertexAttribArray(i);

		offset += attrib->size;
	}
}
