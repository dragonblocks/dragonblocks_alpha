#ifndef _SCENE_H_
#define _SCENE_H_

#include <stdbool.h>
#include <pthread.h>
#include <linmath.h/linmath.h>
#include <dragontype/bintree.h>
#include <dragontype/list.h>
#include <dragontype/number.h>
#include "client/object.h"

extern struct Scene
{
	List objects;
	Bintree transparent_objects;
	pthread_mutex_t mtx;
	GLuint prog;
	GLint loc_model;
	GLint loc_VP;
	GLint loc_daylight;
	GLint loc_fogColor;
	GLint loc_ambientLight;
	GLint loc_lightDir;
	GLint loc_cameraPos;
	GLint max_texture_units;
	mat4x4 VP;
	mat4x4 projection;
	f32 fov;
	f32 render_distance;
} scene;

bool scene_init();
void scene_deinit();
void scene_add_object(Object *obj);
void scene_render(f64 dtime);
void scene_on_resize(int width, int height);
GLuint scene_get_max_texture_units();
void scene_get_view_proj(mat4x4 target);

#endif
