#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <linmath.h/linmath.h>
#include "types.h"

extern struct Camera {
	mat4x4 view;
	vec3 eye, front, right, up;
	struct {
		vec3 front;
		vec3 right;
		vec3 up;
	} movement_dirs;
} camera;

void camera_set_position(v3f32 pos);
void camera_set_angle(f32 yaw, f32 pitch);
void camera_on_resize(int width, int height);

#endif // _CAMERA_H_
