#ifndef _SCENE_H_
#define _SCENE_H_

#include <pthread.h>
#include "list.h"
#include "mesh.h"
#include "shaders.h"

typedef struct Scene
{
	List objects;
	pthread_mutex_t mtx;
} Scene;

Scene *scene_create();
void scene_delete(Scene *scene);

void scene_add_object(Scene *scene, MeshObject *obj);
void scene_render(Scene *scene, ShaderProgram *prog);

#endif
