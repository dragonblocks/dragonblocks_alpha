#include <stdlib.h>
#include <stdio.h>
#include "client/camera.h"
#include "client/client.h"
#include "client/scene.h"
#include "client/shader.h"
#include "day.h"
#include "util.h"

struct Scene scene;

bool scene_init()
{
	scene.objects = list_create(NULL),
	pthread_mutex_init(&scene.mtx, NULL);

	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &scene.max_texture_units);

	char max_texture_units_def[BUFSIZ];
	sprintf(max_texture_units_def, "#define MAX_TEXTURE_UNITS %d\n", scene.max_texture_units);

	if (! shader_program_create(RESSOURCEPATH "shaders/3d", &scene.prog, max_texture_units_def)) {
		fprintf(stderr, "Failed to create 3D shader program\n");
		return false;
	}

	scene.loc_MVP = glGetUniformLocation(scene.prog, "MVP");
	scene.loc_daylight = glGetUniformLocation(scene.prog, "daylight");
	scene.loc_lightDir = glGetUniformLocation(scene.prog, "lightDir");

	GLint texture_indices[scene.max_texture_units];
	for (GLint i = 0; i < scene.max_texture_units; i++)
		texture_indices[i] = i;

	glProgramUniform1iv(scene.prog, glGetUniformLocation(scene.prog, "textures"), scene.max_texture_units, texture_indices);

	scene.fov = 86.1f;
	scene.render_distance = 255.0f;

	return true;
}

static void list_delete_object(void *key, unused void *value, unused void *arg)
{
	object_delete(key);
}

void scene_deinit()
{
	list_clear_func(&scene.objects, &list_delete_object, NULL);
	pthread_mutex_destroy(&scene.mtx);
	glDeleteProgram(scene.prog);
}

void scene_add_object(Object *obj)
{
	pthread_mutex_lock(&scene.mtx);
	list_put(&scene.objects, obj, NULL);
	pthread_mutex_unlock(&scene.mtx);
}

void scene_render()
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
	mat4x4 view_proj;
	mat4x4_mul(view_proj, scene.projection, camera.view);

	vec4 base_sunlight_dir = {0.0f, 0.0f, -1.0f, 1.0f};
	vec4 sunlight_dir;
	mat4x4 sunlight_mat;
	mat4x4_identity(sunlight_mat);
	mat4x4_rotate(sunlight_mat, sunlight_mat, 1.0f, 0.0f, 0.0f, get_sun_angle() + M_PI / 2.0f);
	mat4x4_mul_vec4(sunlight_dir, sunlight_mat, base_sunlight_dir);
#pragma GCC diagnostic pop

	glUseProgram(scene.prog);
	glProgramUniform3f(scene.prog, scene.loc_lightDir, sunlight_dir[0], sunlight_dir[1], sunlight_dir[2]);
	glProgramUniform1f(scene.prog, scene.loc_daylight, get_daylight());

	for (ListPair **pairptr = &scene.objects.first; *pairptr != NULL; ) {
		ListPair *pair = *pairptr;
		Object *obj = pair->key;
		if (obj->remove) {
			pthread_mutex_lock(&scene.mtx);
			*pairptr = pair->next;
			pthread_mutex_unlock(&scene.mtx);
			free(pair);
			object_delete(obj);
		} else {
			object_render(obj, view_proj, scene.loc_MVP);
			pairptr = &pair->next;
		}
	}
}

void scene_on_resize(int width, int height)
{
	mat4x4_perspective(scene.projection, scene.fov / 180.0f * M_PI, (float) width / (float) height, 0.01f, scene.render_distance);
}

GLuint scene_get_max_texture_units()
{
	return scene.max_texture_units;
}
