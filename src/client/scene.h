#ifndef _SCENE_H_
#define _SCENE_H_

#include "client/object.h"

bool scene_init();
void scene_deinit();
void scene_add_object(Object *obj);
void scene_render();
void scene_on_resize(int width, int height);
GLuint scene_get_max_texture_units();

#endif
