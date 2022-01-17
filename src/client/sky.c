#include <stdio.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include "client/camera.h"
#include "client/client.h"
#include "client/cube.h"
#include "client/mesh.h"
#include "client/scene.h"
#include "client/shader.h"
#include "client/sky.h"
#include "client/texture.h"
#include "day.h"

static struct
{
	GLuint skybox_prog;
	GLint skybox_loc_VP;
	GLint skybox_loc_daylight;
	GLuint skybox_textures[2];
	Mesh *skybox_mesh;
	GLuint sun_prog;
	GLint sun_loc_MVP;
	Texture *sun_texture;
	Mesh *sun_mesh;
	GLuint clouds_prog;
	GLint clouds_loc_VP;
	GLint clouds_loc_daylight;
	Mesh *clouds_mesh;
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

typedef struct
{
	GLfloat x, y, z;
} __attribute__((packed)) VertexSkyboxPosition;

typedef struct
{
	VertexSkyboxPosition position;
} __attribute__((packed)) VertexSkybox;

static VertexAttribute skybox_vertex_attributes[2] = {
	// position
	{
		.type = GL_FLOAT,
		.length = 3,
		.size = sizeof(VertexSkyboxPosition),
	},
};

static VertexLayout skybox_vertex_layout = {
	.attributes = skybox_vertex_attributes,
	.count = 1,
	.size = sizeof(VertexSkybox),
};

static VertexSkybox skybox_vertices[6][6];
static VertexSkybox clouds_vertices[6][6];

bool sky_init()
{
	// skybox

	if (! shader_program_create(RESSOURCEPATH "shaders/sky/skybox", &sky.skybox_prog, NULL)) {
		fprintf(stderr, "Failed to create skybox shader program\n");
		return false;
	}

	sky.skybox_loc_VP = glGetUniformLocation(sky.skybox_prog, "VP");
	sky.skybox_loc_daylight = glGetUniformLocation(sky.skybox_prog, "daylight");

	sky.skybox_textures[0] = texture_create_cubemap(RESSOURCEPATH "textures/skybox/day");
	sky.skybox_textures[1] = texture_create_cubemap(RESSOURCEPATH "textures/skybox/night");

	GLint texture_indices[2];
	for (GLint i = 0; i < 2; i++)
		texture_indices[i] = i;

	glProgramUniform1iv(sky.skybox_prog, glGetUniformLocation(sky.skybox_prog, "textures"), 2, texture_indices);

	for (int f = 0; f < 6; f++) {
		for (int v = 0; v < 6; v++) {
			skybox_vertices[f][v].position.x = cube_vertices[f][v].position.x;
			skybox_vertices[f][v].position.y = cube_vertices[f][v].position.y;
			skybox_vertices[f][v].position.z = cube_vertices[f][v].position.z;
		}
	}

	sky.skybox_mesh = mesh_create();
	sky.skybox_mesh->textures = sky.skybox_textures;
	sky.skybox_mesh->textures_count = 2;
	sky.skybox_mesh->free_textures = false;
	sky.skybox_mesh->vertices = skybox_vertices;
	sky.skybox_mesh->vertices_count = 36;
	sky.skybox_mesh->free_vertices = false;
	sky.skybox_mesh->layout = &skybox_vertex_layout;

	// sun

	if (! shader_program_create(RESSOURCEPATH "shaders/sky/sun", &sky.sun_prog, NULL)) {
		fprintf(stderr, "Failed to create sun shader program\n");
		return false;
	}

	sky.sun_loc_MVP = glGetUniformLocation(sky.sun_prog, "MVP");

	sky.sun_texture = texture_load(RESSOURCEPATH "textures/sun.png", false);

	sky.sun_mesh = mesh_create();
	sky.sun_mesh->textures = &sky.sun_texture->id;
	sky.sun_mesh->textures_count = 1;
	sky.sun_mesh->free_textures = false;
	sky.sun_mesh->vertices = sun_vertices;
	sky.sun_mesh->vertices_count = 6;
	sky.sun_mesh->free_vertices = false;
	sky.sun_mesh->layout = &sun_vertex_layout;

	// clouds

	if (! shader_program_create(RESSOURCEPATH "shaders/sky/clouds", &sky.clouds_prog, NULL)) {
		fprintf(stderr, "Failed to create clouds shader program\n");
		return false;
	}

	sky.clouds_loc_VP = glGetUniformLocation(sky.clouds_prog, "VP");
	sky.clouds_loc_daylight = glGetUniformLocation(sky.clouds_prog, "daylight");

	for (int f = 0; f < 6; f++) {
		for (int v = 0; v < 6; v++) {
			clouds_vertices[f][v].position.x = cube_vertices[f][v].position.x;
			clouds_vertices[f][v].position.y = fmax(cube_vertices[f][v].position.y, 0.0);
			clouds_vertices[f][v].position.z = cube_vertices[f][v].position.z;
		}
	}

	sky.clouds_mesh = mesh_create();
	sky.clouds_mesh->textures = sky.skybox_textures;
	sky.clouds_mesh->textures_count = 1;
	sky.clouds_mesh->free_textures = false;
	sky.clouds_mesh->vertices = clouds_vertices;
	sky.clouds_mesh->vertices_count = 36;
	sky.clouds_mesh->free_vertices = false;
	sky.clouds_mesh->layout = &skybox_vertex_layout;

	return true;
}

void sky_deinit()
{
	glDeleteProgram(sky.skybox_prog);
	glDeleteTextures(1, &sky.skybox_textures[0]);
	glDeleteTextures(1, &sky.skybox_textures[1]);
	mesh_delete(sky.skybox_mesh);

	glDeleteProgram(sky.sun_prog);
	mesh_delete(sky.sun_mesh);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
void sky_render()
{
	f64 daylight = get_daylight();
	f64 sun_angle = get_sun_angle();

	vec3 sun_pos = {0.0f, cos(sun_angle), sin(sun_angle)};
	vec3_norm(sun_pos, sun_pos);
	vec3_scale(sun_pos, sun_pos, 5.0f);

	mat4x4 model;
	mat4x4_translate(model, sun_pos[0], sun_pos[1], sun_pos[2]);
	mat4x4_rotate(model, model, 1.0f, 0.0f, 0.0f, sun_angle + M_PI / 2.0f);

	mat4x4 view;
	mat4x4_dup(view, camera.view);
	view[3][0] = 0.0f;
	view[3][1] = 0.0f;
	view[3][2] = 0.0f;

	mat4x4 VP;
	mat4x4_mul(VP, scene.projection, view);

	mat4x4 MVP;
	mat4x4_mul(MVP, VP, model);

	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);

	glUseProgram(sky.skybox_prog);
	glUniformMatrix4fv(sky.skybox_loc_VP, 1, GL_FALSE, VP[0]);
	glUniform1f(sky.skybox_loc_daylight, daylight);
	mesh_render(sky.skybox_mesh);

	glUseProgram(sky.sun_prog);
	glUniformMatrix4fv(sky.sun_loc_MVP, 1, GL_FALSE, MVP[0]);
	mesh_render(sky.sun_mesh);

	glUseProgram(sky.clouds_prog);
	glUniformMatrix4fv(sky.clouds_loc_VP, 1, GL_FALSE, VP[0]);
	glUniform1f(sky.clouds_loc_daylight, daylight);
	mesh_render(sky.clouds_mesh);

	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
}
#pragma GCC diagnostic pop
