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

Mesh *create_mesh(const GLvoid *vertices, GLsizei size);
void delete_mesh(Mesh *mesh);

#endif
