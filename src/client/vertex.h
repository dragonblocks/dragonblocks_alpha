#ifndef _VERTEX_H_
#define _VERTEX_H_

#include <GL/glew.h>
#include <GL/gl.h>

typedef struct
{
	GLenum type;
	GLsizei length;
	GLsizei size;
} VertexAttribute;

typedef struct
{
	VertexAttribute *attributes;
	GLuint count;
	GLsizei size;
} VertexLayout;

void vertex_layout_configure(VertexLayout *layout);

#endif
