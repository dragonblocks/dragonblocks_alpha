#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <linmath.h/linmath.h>
#include "client/camera.h"
#include "client/client.h"
#include "client/scene.h"
#include "client/shader.h"
#include "list.h"
#include "util.h"

static struct
{
	List objects;
	pthread_mutex_t mtx;
	GLuint prog;
	GLint loc_model;
	GLint loc_view;
	GLint loc_projection;
	mat4x4 projection;
	f32 fov;
	f32 render_distance;
} scene;

bool scene_init()
{
	scene.objects = list_create(NULL),
	pthread_mutex_init(&scene.mtx, NULL);

	if (! shader_program_create(RESSOURCEPATH "shaders/3d", &scene.prog)) {
		fprintf(stderr, "Failed to create 3D shader program\n");
		return false;
	}

	scene.loc_model = glGetUniformLocation(scene.prog, "model");
	scene.loc_view = glGetUniformLocation(scene.prog, "view");
	scene.loc_projection = glGetUniformLocation(scene.prog, "projection");

	scene.fov = 86.1f;
	scene.render_distance = 1000.0f;

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
	glUseProgram(scene.prog);
	camera_enable(scene.loc_view);
	glUniformMatrix4fv(scene.loc_projection, 1, GL_FALSE, scene.projection[0]);

	glActiveTexture(GL_TEXTURE0);

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
			object_render(obj, scene.loc_model);
			pairptr = &pair->next;
		}
	}
}

void scene_on_resize(int width, int height)
{
	mat4x4_perspective(scene.projection, scene.fov / 180.0f * M_PI, (float) width / (float) height, 0.01f, scene.render_distance);
}
