#include <math.h>
#include "client.h"
#include "camera.h"

static struct
{
	mat4x4 view, projection;
	GLFWwindow *window;
	ShaderProgram *prog;

	vec3 eye, front, right, up;
} camera;

static vec3 world_up = {0.0f, 1.0f, 0.0f};

struct movement_dirs movement_dirs;

static void update_camera()
{
	vec3 center;
	vec3_add(center, camera.eye, camera.front);

	mat4x4_look_at(camera.view, camera.eye, center, camera.up);
}

void camera_enable()
{
	glUniformMatrix4fv(camera.prog->loc_view, 1, GL_FALSE, camera.view[0]);
	glUniformMatrix4fv(camera.prog->loc_projection, 1, GL_FALSE, camera.projection[0]);
}

void set_camera_position(v3f pos)
{
	camera.eye[0] = pos.x;
	camera.eye[1] = pos.y;
	camera.eye[2] = pos.z;

	update_camera();
}

void set_camera_angle(f32 yaw, f32 pitch)
{
	camera.front[0] = movement_dirs.front[0] = cos(yaw) * cos(pitch);
	camera.front[1] = sin(pitch);
	camera.front[2] = movement_dirs.front[2] = sin(yaw) * cos(pitch);

	vec3_norm(camera.front, camera.front);
	vec3_norm(movement_dirs.front, movement_dirs.front);

	vec3_mul_cross(camera.right, camera.front, world_up);
	movement_dirs.right[0] = camera.right[0];
	movement_dirs.right[2] = camera.right[2];

	vec3_norm(camera.right, camera.right);
	vec3_norm(movement_dirs.right, movement_dirs.right);

	vec3_mul_cross(camera.up, camera.right, camera.front);
	vec3_norm(camera.up, camera.up);

	update_camera();
}

void set_window_size(int width, int height)
{
	mat4x4_perspective(camera.projection, 86.1f / 180.0f * M_PI, (float) width / (float) height, 0.01f, 1000.0f);
}

void init_camera(GLFWwindow *window, ShaderProgram *prog)
{
	camera.window = window;
	camera.prog = prog;
}
