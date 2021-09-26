#include <stdio.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include "client/camera.h"
#include "client/client.h"
#include "client/mesh.h"
#include "client/scene.h"
#include "client/shader.h"
#include "client/sky.h"
#include "client/texture.h"
#include "day.h"

static struct
{
	GLuint sun_prog;
	GLint sun_loc_MVP;
	Texture *sun_texture;
	Mesh *sun_mesh;
} sky;

typedef struct
{
	GLfloat x, y, z;
} __attribute__((packed)) VertexSunPosition;

typedef struct
{
	GLfloat s, t;
} __attribute__((packed)) VertexSunTextureCoordinates;

typedef struct
{
	VertexSunPosition position;
	VertexSunTextureCoordinates textureCoordinates;
} __attribute__((packed)) VertexSun;

static VertexAttribute sun_vertex_attributes[2] = {
	// position
	{
		.type = GL_FLOAT,
		.length = 3,
		.size = sizeof(VertexSunPosition),
	},
	// textureCoordinates
	{
		.type = GL_FLOAT,
		.length = 2,
		.size = sizeof(VertexSunTextureCoordinates),
	},
};

static VertexLayout sun_vertex_layout = {
	.attributes = sun_vertex_attributes,
	.count = 2,
	.size = sizeof(VertexSun),
};

static VertexSun sun_vertices[6] = {
	{{-0.5, -0.5, +0.0}, {+0.0, +0.0}},
	{{+0.5, -0.5, +0.0}, {+1.0, +0.0}},
	{{+0.5, +0.5, +0.0}, {+1.0, +1.0}},
	{{+0.5, +0.5, +0.0}, {+1.0, +1.0}},
	{{-0.5, +0.5, +0.0}, {+0.0, +1.0}},
	{{-0.5, -0.5, +0.0}, {+0.0, +0.0}},
};

bool sky_init()
{
	if (! shader_program_create(RESSOURCEPATH "shaders/sky/sun", &sky.sun_prog, NULL)) {
		fprintf(stderr, "Failed to create sun shader program\n");
		return false;
	}

	sky.sun_texture = texture_get(RESSOURCEPATH "textures/sun.png");

	sky.sun_mesh = mesh_create();
	sky.sun_mesh->textures = &sky.sun_texture->id;
	sky.sun_mesh->textures_count = 1;
	sky.sun_mesh->free_textures = false;
	sky.sun_mesh->vertices = sun_vertices;
	sky.sun_mesh->vertices_count = 6;
	sky.sun_mesh->free_vertices = false;
	sky.sun_mesh->layout = &sun_vertex_layout;

	return true;
}

void sky_deinit()
{
	glDeleteProgram(sky.sun_prog);
	mesh_delete(sky.sun_mesh);
}

void sky_clear()
{
	// sky color: #030A1A at night -> #87CEEB at day
	f64 daylight = get_daylight();
	glClearColor((daylight * (0x87 - 0x03) + 0x03) / 0xFF, (daylight * (0xCE - 0x0A) + 0x0A) / 0xFF, (daylight * (0xEB - 0x1A) + 0x1A) / 0xFF, 1.0f);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
void sky_render()
{
	f64 sun_angle = get_sun_angle();

	vec3 sun_pos = {0.0f, cos(sun_angle), sin(sun_angle)};
	vec3_norm(sun_pos, sun_pos);
	vec3_scale(sun_pos, sun_pos, 5.0f);
	vec3_add(sun_pos, sun_pos, camera.eye);

	vec3 forward;
	vec3_sub(forward, camera.eye, sun_pos);
	vec3_norm(forward, forward);

	vec3 up = {0.0f, 0.0f, 0.0f};
	if (fabs(forward[0]) == 0.0f && fabs(forward[2]) == 0.0f) {
		if (forward[2] > 0.0f)
			up[2] = -1.0f;
		else
			up[2] = 1.0f;
	} else {
		up[1] = 1.0f;
	}

	vec3 left;
	vec3_mul_cross(left, up, forward);
	vec3_norm(left, left);

	vec3_mul_cross(up, forward, left);

	mat4x4 model = {
		{left[0], up[0], forward[0], 0.0f},
		{left[1], up[1], forward[1], 0.0f},
		{left[2], up[2], forward[2], 0.0f},
		{sun_pos[0], sun_pos[1], sun_pos[2], 1.0f},
	};

	mat4x4_rotate(model, model, 0.0f, 0.0f, 1.0f, M_PI / 4.0f);

	mat4x4 VP, MVP;

	mat4x4_mul(VP, scene.projection, camera.view);
	mat4x4_mul(MVP, VP, model);

	glUseProgram(sky.sun_prog);
	glUniformMatrix4fv(sky.sun_loc_MVP, 1, GL_FALSE, MVP[0]);
	glDepthMask(GL_FALSE);
	mesh_render(sky.sun_mesh);
	glDepthMask(GL_TRUE);
}
#pragma GCC diagnostic pop
