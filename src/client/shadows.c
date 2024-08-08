#include "client/camera.h"
#include "client/client_config.h"
#include "client/gui.h"
#include "client/model.h"
#include "client/opengl.h"
#include "client/shader.h"
#include "client/shadows.h"
#include "client/window.h"
#include "common/day.h"

static GLuint shadow_map;
static GLuint shadow_map_fbo;
static GLint loc_model;
static GLint loc_VP;
static GLuint shader_prog;
static mat4x4 light_view_proj;

static GUIElement *shadow_gui;
static Texture shadow_texture;

static unsigned int shadow_map_size = 5000;

void shadows_init()
{
	shader_prog = shader_program_create(ASSET_PATH "shaders/shadow", NULL);
	loc_model = glGetUniformLocation(shader_prog, "model"); GL_DEBUG
	loc_VP = glGetUniformLocation(shader_prog, "VP"); GL_DEBUG

	glGenTextures(1, &shadow_map); GL_DEBUG
	glBindTexture(GL_TEXTURE_2D, shadow_map); GL_DEBUG
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		shadow_map_size, shadow_map_size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL); GL_DEBUG
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); GL_DEBUG
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); GL_DEBUG
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); GL_DEBUG
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); GL_DEBUG
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); GL_DEBUG
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER); GL_DEBUG
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (vec4) { 1.0, 1.0, 1.0, 1.0 }); GL_DEBUG

	glGenFramebuffers(1, &shadow_map_fbo); GL_DEBUG
	glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_fbo); GL_DEBUG
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_map, 0); GL_DEBUG
	glDrawBuffer(GL_NONE); GL_DEBUG
	glReadBuffer(GL_NONE); GL_DEBUG
	glBindFramebuffer(GL_FRAMEBUFFER, 0); GL_DEBUG

	shadow_texture.txo = shadow_map;
	shadow_texture.width = window.width;
	shadow_texture.height = window.height;
}

void shadows_deinit()
{
	// TODO
}

GLuint shadows_get_map()
{
	return shadow_map;
}

void shadows_get_light_view_proj(GLuint shader, GLint loc)
{
	glProgramUniformMatrix4fv(shader, loc, 1, GL_FALSE, light_view_proj[0]); GL_DEBUG
}

void shadows_set_model(mat4x4 model)
{
	glProgramUniformMatrix4fv(shader_prog, loc_model, 1, GL_FALSE, model[0]); GL_DEBUG
}

void shadows_render_map()
{
	float dist = 50; // client_config.view_distance;
	f64 sun_angle = get_sun_angle();

	vec3 sun_pos = {0.0f, cos(sun_angle), sin(sun_angle)};
	vec3_scale(sun_pos, sun_pos, dist);
	vec3_add(sun_pos, sun_pos, camera.eye);

	mat4x4 view;
	mat4x4_look_at(view, sun_pos, camera.eye, (vec3) { 0.0f, 1.0f, 0.0f });
	mat4x4 proj;
	mat4x4_ortho(proj, -dist, dist, -dist, dist, 1.0, dist*2);

	mat4x4_mul(light_view_proj, proj, view);

	glUseProgram(shader_prog); GL_DEBUG
	shadows_get_light_view_proj(shader_prog, loc_VP);

	glCullFace(GL_FRONT);
	glViewport(0, 0, shadow_map_size, shadow_map_size); GL_DEBUG
	glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_fbo); GL_DEBUG
	glClear(GL_DEPTH_BUFFER_BIT); GL_DEBUG
	model_scene_render_shadows();
	glBindFramebuffer(GL_FRAMEBUFFER, 0); GL_DEBUG
	window_set_viewport();
	glCullFace(GL_BACK);

	if (!shadow_gui) shadow_gui = gui_add(NULL, (GUIElementDef) {
		.pos = {0.8f, 0.0f},
		.z_index = 0.80f,
		.offset = {0, 0},
		.margin = {0, 0},
		.align = {0.0f, 0.0f},
		.scale = {0.2f, 0.2f},
		.scale_type = SCALE_PARENT,
		.affect_parent_scale = false,
		.text = NULL,
		.image = &shadow_texture,
		.text_color = (v4f32) {1.0f, 1.0f, 1.0f, 1.0f},
		.bg_color = (v4f32) {0.0f, 0.0f, 0.0f, 0.0f},
	});

}
