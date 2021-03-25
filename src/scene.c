#include <stdlib.h>
#include "scene.h"

Scene *scene_create()
{
	Scene *scene = malloc(sizeof(Scene));
	scene->meshes = list_create(NULL),
	pthread_mutex_init(&scene->mtx, NULL);

	return scene;
}

void scene_delete(Scene *scene)
{
	ITERATE_LIST(&scene->meshes, pair) mesh_delete(pair->value);
	list_clear(&scene->meshes);
	pthread_mutex_destroy(&scene->mtx);
	free(scene);
}

void scene_add_mesh(Scene *scene, Mesh *mesh)
{
	list_put(&scene->meshes, mesh, NULL);
}

void scene_remove_mesh(Scene *scene, Mesh *mesh)
{
	pthread_mutex_lock(&scene->mtx);
	list_delete(&scene->meshes, mesh);
	pthread_mutex_unlock(&scene->mtx);
	mesh_delete(mesh);
}

