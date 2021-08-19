#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <linmath.h/linmath.h>
#include "types.h"

void camera_set_position(v3f32 pos);
void camera_set_angle(f32 yaw, f32 pitch);
void camera_on_resize(int width, int height);
void camera_enable(GLint loc_view);

extern struct camera_movement_dirs
{
	vec3 front;
	vec3 right;
} camera_movement_dirs;

#endif
