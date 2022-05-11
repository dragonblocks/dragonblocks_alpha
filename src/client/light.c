#include <linmath.h>
#include "client/camera.h"
#include "client/opengl.h"
#include "client/light.h"
#include "common/day.h"

void light_shader_locate(LightShader *shader)
{
	shader->loc_daylight = glGetUniformLocation(shader->prog, "daylight"); GL_DEBUG
	shader->loc_fogColor = glGetUniformLocation(shader->prog, "fogColor"); GL_DEBUG
	shader->loc_ambientLight = glGetUniformLocation(shader->prog, "ambientLight"); GL_DEBUG
	shader->loc_lightDir = glGetUniformLocation(shader->prog, "lightDir"); GL_DEBUG
	shader->loc_cameraPos = glGetUniformLocation(shader->prog, "cameraPos"); GL_DEBUG
}

void light_shader_update(LightShader *shader)
{
	vec4 base_sunlight_dir = {0.0f, 0.0f, -1.0f, 1.0f};
	vec4 sunlight_dir;
	mat4x4 sunlight_mat;
	mat4x4_identity(sunlight_mat);

	mat4x4_rotate(sunlight_mat, sunlight_mat, 1.0f, 0.0f, 0.0f, get_sun_angle() + M_PI / 2.0f);
	mat4x4_mul_vec4(sunlight_dir, sunlight_mat, base_sunlight_dir);

	f32 daylight = get_daylight();
	f32 ambient_light = f32_mix(0.3f, 0.7f, daylight);
	v3f32 fog_color = v3f32_mix((v3f32) {0x03, 0x0A, 0x1A}, (v3f32) {0x87, 0xCE, 0xEB}, daylight);

	glProgramUniform1f(shader->prog, shader->loc_daylight, daylight); GL_DEBUG
	glProgramUniform3f(shader->prog, shader->loc_fogColor, fog_color.x / 0xFF * ambient_light, fog_color.y / 0xFF * ambient_light, fog_color.z / 0xFF * ambient_light); GL_DEBUG
	glProgramUniform1f(shader->prog, shader->loc_ambientLight, ambient_light); GL_DEBUG
	glProgramUniform3f(shader->prog, shader->loc_lightDir, sunlight_dir[0], sunlight_dir[1], sunlight_dir[2]); GL_DEBUG
	glProgramUniform3f(shader->prog, shader->loc_cameraPos, camera.eye[0], camera.eye[1], camera.eye[2]); GL_DEBUG
}
