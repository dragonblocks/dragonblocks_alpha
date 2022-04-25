#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <linmath.h>
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

void camera_init();
void camera_set_position(v3f32 pos);
void camera_set_angle(f32 yaw, f32 pitch);

#endif // _CAMERA_H_
