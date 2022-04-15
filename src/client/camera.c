#include <math.h>
#include "client/camera.h"
#include "client/client.h"

struct Camera camera;

static const vec3 world_up = {0.0f, 1.0f, 0.0f};

static void camera_update()
{
	vec3 center;
	vec3_add(center, camera.eye, camera.front);

	mat4x4_look_at(camera.view, camera.eye, center, camera.up);
}

void camera_set_position(v3f32 pos)
{
	camera.eye[0] = pos.x;
	camera.eye[1] = pos.y;
	camera.eye[2] = pos.z;

	camera_update();
}

void camera_set_angle(f32 yaw, f32 pitch)
{
	camera.front[0] = camera.movement_dirs.front[0] = cos(yaw) * cos(pitch);
	camera.front[1] = sin(pitch);
	camera.front[2] = camera.movement_dirs.front[2] = sin(yaw) * cos(pitch);

	vec3_norm(camera.front, camera.front);
	vec3_norm(camera.movement_dirs.front, camera.movement_dirs.front);

	vec3_mul_cross(camera.right, camera.front, world_up);
	camera.movement_dirs.right[0] = camera.right[0];
	camera.movement_dirs.right[2] = camera.right[2];

	vec3_norm(camera.right, camera.right);
	vec3_norm(camera.movement_dirs.right, camera.movement_dirs.right);

	vec3_mul_cross(camera.up, camera.right, camera.front);
	vec3_norm(camera.up, camera.up);

	camera.movement_dirs.up[0] = world_up[0];
	camera.movement_dirs.up[1] = world_up[1];
	camera.movement_dirs.up[2] = world_up[2];

	camera_update();
}
