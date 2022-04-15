#include <stdio.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include "client/camera.h"
#include "client/client.h"
#include "client/cube.h"
#include "client/mesh.h"
#include "client/shader.h"
#include "client/sky.h"
#include "client/texture.h"
#include "client/window.h"
#include "day.h"

static GLuint sun_prog;
static GLint sun_loc_MVP;
static GLuint sun_texture;
typedef struct {
	v3f32 position;
	v2f32 textureCoordinates;
} __attribute__((packed)) SunVertex;
static Mesh sun_mesh = {
	.layout = &(VertexLayout) {
		.attributes = (VertexAttribute[]) {
			{GL_FLOAT, 3, sizeof(v3f32)}, // position
			{GL_FLOAT, 2, sizeof(v2f32)}, // textureCoordinates
		},
		.count = 2,
		.size = sizeof(SunVertex),
	},
	.vao = 0,
	.vbo = 0,
	.data = (SunVertex[]) {
		{{-0.5, -0.5, +0.0}, {+0.0, +0.0}},
		{{+0.5, -0.5, +0.0}, {+1.0, +0.0}},
		{{+0.5, +0.5, +0.0}, {+1.0, +1.0}},
		{{+0.5, +0.5, +0.0}, {+1.0, +1.0}},
		{{-0.5, +0.5, +0.0}, {+0.0, +1.0}},
		{{-0.5, -0.5, +0.0}, {+0.0, +0.0}},
	},
	.count = 6,
	.free_data = false,
};

static GLuint skybox_prog;
static GLint skybox_loc_VP;
static GLint skybox_loc_daylight;
static GLuint skybox_texture_day;
static GLuint skybox_texture_night;
typedef struct {
	v3f32 position;
} __attribute__((packed)) SkyboxVertex;
static Mesh skybox_mesh = {
	.layout = &(VertexLayout) {
		.attributes = (VertexAttribute[]) {
			{GL_FLOAT, 3, sizeof(v3f32)}, // position
		},
		.count = 1,
		.size = sizeof(SkyboxVertex),
	},
	.vao = 0,
	.vbo = 0,
	.data = NULL,
	.count = 36,
	.free_data = false,
};

static GLuint clouds_prog;
static GLint clouds_loc_VP;
static GLint clouds_loc_daylight;
static Mesh clouds_mesh;

bool sky_init()
{
	clouds_mesh = skybox_mesh;
	SkyboxVertex skybox_vertices[6][6], clouds_vertices[6][6];
	for (int f = 0; f < 6; f++) {
		for (int v = 0; v < 6; v++) {
			skybox_vertices[f][v].position =
				clouds_vertices[f][v].position =
				cube_vertices[f][v].position;

			clouds_vertices[f][v].position.y = fmax(clouds_vertices[f][v].position.y, 0.0);
		}
	}

	// skybox

	if (!shader_program_create(RESSOURCE_PATH "shaders/sky/skybox", &skybox_prog, NULL)) {
		fprintf(stderr, "[error] failed to create skybox shader program\n");
		return false;
	}

	glProgramUniform1iv(skybox_prog, glGetUniformLocation(skybox_prog, "textures"), 2, (GLint[]) {0, 1});

	skybox_loc_VP = glGetUniformLocation(skybox_prog, "VP");
	skybox_loc_daylight = glGetUniformLocation(skybox_prog, "daylight");
	skybox_texture_day = texture_load_cubemap(RESSOURCE_PATH "textures/skybox/day")->txo;
	skybox_texture_night = texture_load_cubemap(RESSOURCE_PATH "textures/skybox/night")->txo;
	skybox_mesh.data = skybox_vertices;
	mesh_upload(&skybox_mesh);

	// sun

	if (!shader_program_create(RESSOURCE_PATH "shaders/sky/sun", &sun_prog, NULL)) {
		fprintf(stderr, "[error] failed to create sun shader program\n");
		return false;
	}

	sun_loc_MVP = glGetUniformLocation(sun_prog, "MVP");
	sun_texture = texture_load(RESSOURCE_PATH "textures/sun.png", false)->txo;

	// clouds

	if (!shader_program_create(RESSOURCE_PATH "shaders/sky/clouds", &clouds_prog, NULL)) {
		fprintf(stderr, "[error] failed to create clouds shader program\n");
		return false;
	}

	clouds_loc_VP = glGetUniformLocation(clouds_prog, "VP");
	clouds_loc_daylight = glGetUniformLocation(clouds_prog, "daylight");
	clouds_mesh.data = clouds_vertices;
	mesh_upload(&clouds_mesh);

	return true;
}

void sky_deinit()
{
	glDeleteProgram(sun_prog);
	mesh_destroy(&sun_mesh);

	glDeleteProgram(skybox_prog);
	mesh_destroy(&skybox_mesh);

	glDeleteProgram(clouds_prog);
	mesh_destroy(&clouds_mesh);
}

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

	mat4x4 vp;
	mat4x4_mul(vp, window.projection, view);

	mat4x4 mvp;
	mat4x4_mul(mvp, vp, model);

	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LEQUAL);

	glUseProgram(skybox_prog);
	glUniformMatrix4fv(skybox_loc_VP, 1, GL_FALSE, vp[0]);
	glUniform1f(skybox_loc_daylight, daylight);
	glBindTextureUnit(0, skybox_texture_day);
	glBindTextureUnit(1, skybox_texture_night);
	mesh_render(&skybox_mesh);

	glUseProgram(sun_prog);
	glUniformMatrix4fv(sun_loc_MVP, 1, GL_FALSE, mvp[0]);
	glBindTextureUnit(0, sun_texture);
	mesh_render(&sun_mesh);

	glUseProgram(clouds_prog);
	glUniformMatrix4fv(clouds_loc_VP, 1, GL_FALSE, vp[0]);
	glUniform1f(clouds_loc_daylight, daylight);
	glBindTextureUnit(0, skybox_texture_day);
	mesh_render(&clouds_mesh);

	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
}
