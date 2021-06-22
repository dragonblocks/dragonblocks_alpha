#include <stdlib.h>
#include "scene.h"

Scene *scene_create()
{
	Scene *scene = malloc(sizeof(Scene));
	scene->objects = list_create(NULL),
	pthread_mutex_init(&scene->mtx, NULL);
	return scene;
}

static void list_delete_mesh(void *key, __attribute__((unused)) void *value, __attribute__((unused)) void *unused)
{
	meshobject_delete(key);
}

void scene_delete(Scene *scene)
{
	list_clear_func(&scene->objects, &list_delete_mesh, NULL);
	pthread_mutex_destroy(&scene->mtx);
	free(scene);
}

void scene_add_object(Scene *scene, MeshObject *obj)
{
	pthread_mutex_lock(&scene->mtx);
	list_put(&scene->objects, obj, NULL);
	pthread_mutex_unlock(&scene->mtx);
}

void scene_render(Scene *scene, ShaderProgram *prog)
{
	for (ListPair **pairptr = &scene->objects.first; *pairptr != NULL; ) {
		ListPair *pair = *pairptr;
		MeshObject *obj = pair->key;
		if (obj->remove) {
			pthread_mutex_lock(&scene->mtx);
			*pairptr = pair->next;
			pthread_mutex_unlock(&scene->mtx);
			free(pair);
			meshobject_delete(obj);
		} else {
			meshobject_render(obj, prog);
			pairptr = &pair->next;
		}
	}
}
