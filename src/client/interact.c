#include <linmath.h/linmath.h>
#include <stdio.h>
#include "client/camera.h"
#include "client/client_node.h"
#include "client/cube.h"
#include "client/frustum.h"
#include "client/gl_debug.h"
#include "client/interact.h"
#include "client/mesh.h"
#include "client/raycast.h"
#include "client/shader.h"

static bool pointed;
static v3s32 node_pos;
static GLuint shader_prog;
static GLint loc_MVP;
static GLint loc_color;
static mat4x4 model;

typedef struct {
	v3f32 position;
} __attribute__((packed)) SelectionVertex;
static Mesh selection_mesh = {
	.layout = &(VertexLayout) {
		.attributes = (VertexAttribute[]) {
			{GL_FLOAT, 3, sizeof(v3f32)},
		},
		.count = 1,
		.size = sizeof(SelectionVertex),
	},
	.vao = 0,
	.vbo = 0,
	.data = NULL,
	.count = 36,
	.free_data = false,
};

bool interact_init()
{
	if (!shader_program_create(RESSOURCE_PATH "shaders/3d/selection", &shader_prog, NULL)) {
		fprintf(stderr, "[error] failed to create selection shader program\n");
		return false;
	}

	loc_MVP = glGetUniformLocation(shader_prog, "MVP"); GL_DEBUG
	loc_color = glGetUniformLocation(shader_prog, "color"); GL_DEBUG

	SelectionVertex vertices[6][6];
	for (int f = 0; f < 6; f++)
		for (int v = 0; v < 6; v++)
			vertices[f][v].position = v3f32_scale(cube_vertices[f][v].position, 1.1f);

	selection_mesh.data = vertices;
	mesh_upload(&selection_mesh);

	return true;
}

void interact_deinit()
{
	glDeleteProgram(shader_prog); GL_DEBUG
	mesh_destroy(&selection_mesh);
}

void interact_tick()
{
	v3s32 old_node_pos = node_pos;

	NodeType node;
	if ((pointed = raycast(
			(v3f64) {camera.eye  [0], camera.eye  [1], camera.eye  [2]}, 
			(v3f64) {camera.front[0], camera.front[1], camera.front[2]},
			5, &node_pos, &node)) && !v3s32_equals(node_pos, old_node_pos)) {
		mat4x4_translate(model, node_pos.x, node_pos.y, node_pos.z);
		v3f32 *color = &client_node_definitions[node].selection_color;
		glProgramUniform3f(shader_prog, loc_color, color->x, color->y, color->z); GL_DEBUG
	}
}

void interact_render()
{
	if (!pointed)
		return;

	mat4x4 mvp;
	mat4x4_mul(mvp, frustum, model);

	glUseProgram(shader_prog); GL_DEBUG
	glUniformMatrix4fv(loc_MVP, 1, GL_FALSE, mvp[0]); GL_DEBUG
	mesh_render(&selection_mesh);
}
