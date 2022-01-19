#include <stdlib.h>
#include <stdio.h>
#include "client/camera.h"
#include "client/client.h"
#include "client/frustum.h"
#include "client/scene.h"
#include "client/shader.h"
#include "day.h"
#include "util.h"

struct Scene scene;

static int bintree_compare_f32(void *v1, void *v2, unused Bintree *tree)
{
	f32 diff = (*(f32 *) v2) - (*(f32 *) v1);
	return CMPBOUNDS(diff);
}

bool scene_init()
{
	scene.objects = list_create(NULL);
	scene.transparent_objects = bintree_create(sizeof(f32), &bintree_compare_f32);
	pthread_mutex_init(&scene.mtx, NULL);

	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &scene.max_texture_units);

	char max_texture_units_def[BUFSIZ];
	sprintf(max_texture_units_def, "#define MAX_TEXTURE_UNITS %d\n", scene.max_texture_units);

	if (! shader_program_create(RESSOURCEPATH "shaders/3d", &scene.prog, max_texture_units_def)) {
		fprintf(stderr, "Failed to create 3D shader program\n");
		return false;
	}

	scene.loc_model = glGetUniformLocation(scene.prog, "model");
	scene.loc_VP = glGetUniformLocation(scene.prog, "VP");
	scene.loc_daylight = glGetUniformLocation(scene.prog, "daylight");
	scene.loc_fogColor = glGetUniformLocation(scene.prog, "fogColor");
	scene.loc_ambientLight = glGetUniformLocation(scene.prog, "ambientLight");
	scene.loc_lightDir = glGetUniformLocation(scene.prog, "lightDir");
	scene.loc_cameraPos = glGetUniformLocation(scene.prog, "cameraPos");

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

static void bintree_render_object(BintreeNode *node, unused void *arg)
{
	object_render(node->value);
}

void scene_render(f64 dtime)
{
	mat4x4_mul(scene.VP, scene.projection, camera.view);

	vec4 base_sunlight_dir = {0.0f, 0.0f, -1.0f, 1.0f};
	vec4 sunlight_dir;
	mat4x4 sunlight_mat;
	mat4x4_identity(sunlight_mat);

	mat4x4_rotate(sunlight_mat, sunlight_mat, 1.0f, 0.0f, 0.0f, get_sun_angle() + M_PI / 2.0f);
	mat4x4_mul_vec4(sunlight_dir, sunlight_mat, base_sunlight_dir);

	frustum_update(scene.VP);

	f32 daylight = get_daylight();
	f32 ambient_light = f32_mix(0.3f, 0.7f, daylight);
	v3f32 fog_color = v3f32_mix((v3f32) {0x03, 0x0A, 0x1A}, (v3f32) {0x87, 0xCE, 0xEB}, daylight);

	glUseProgram(scene.prog);
	glUniformMatrix4fv(scene.loc_VP, 1, GL_FALSE, scene.VP[0]);
	glUniform3f(scene.loc_lightDir, sunlight_dir[0], sunlight_dir[1], sunlight_dir[2]);
	glUniform3f(scene.loc_cameraPos, camera.eye[0], camera.eye[1], camera.eye[2]);
	glUniform1f(scene.loc_daylight, daylight);
	glUniform3f(scene.loc_fogColor, fog_color.x / 0xFF * ambient_light, fog_color.y / 0xFF * ambient_light, fog_color.z / 0xFF * ambient_light);
	glUniform1f(scene.loc_ambientLight, ambient_light);

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
			f32 distance = sqrt(pow(obj->pos.x - camera.eye[0], 2) + pow(obj->pos.y - camera.eye[1], 2) + pow(obj->pos.z - camera.eye[2], 2));
			if (distance < scene.render_distance && object_before_render(obj, dtime)) {
				if (obj->transparent)
					bintree_insert(&scene.transparent_objects, &distance, obj);
				else
					object_render(obj);
			}
			pairptr = &pair->next;
		}
	}

	bintree_traverse(&scene.transparent_objects, BTT_INORDER, &bintree_render_object, NULL);
	bintree_clear(&scene.transparent_objects, NULL, NULL);
}

void scene_on_resize(int width, int height)
{
	mat4x4_perspective(scene.projection, scene.fov / 180.0f * M_PI, (float) width / (float) height, 0.01f, scene.render_distance + 28.0f);
}

GLuint scene_get_max_texture_units()
{
	return scene.max_texture_units;
}
