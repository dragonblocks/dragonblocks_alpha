#ifndef _SHADOWS_H_
#define _SHADOWS_H_

#include <linmath.h>
#include "client/opengl.h"

void shadows_init();
void shadows_deinit();

void shadows_render_map();
GLuint shadows_get_map();

void shadows_get_light_view_proj(GLuint shader, GLint loc);
void shadows_set_model(mat4x4 model);

#endif
