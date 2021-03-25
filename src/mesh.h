#ifndef _MESH_H_
#define _MESH_H_

#include <GL/glew.h>
#include <GL/gl.h>
#include <linmath.h/linmath.h>

typedef struct
{
	vec3 pos, rot, scale;
	float angle;
	mat4x4 transform;
	GLuint VAO, VBO;
} Mesh;

Mesh *mesh_create(const GLvoid *vertices, GLsizei size);
void mesh_delete(Mesh *mesh);

#endif
