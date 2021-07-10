#include <math.h>
#include "camera.h"
#include "client.h"
#include "input.h"

static struct
{
	GLFWwindow *window;
	Client *client;
} input;

static void cursor_pos_callback(__attribute__((unused)) GLFWwindow* window, double current_x, double current_y)
{
	static double last_x, last_y = 0.0;

	double delta_x = current_x - last_x;
	double delta_y = current_y - last_y;
	last_x = current_x;
	last_y = current_y;

	input.client->player.yaw += (f32) delta_x * M_PI / 180.0f / 8.0f;
	input.client->player.pitch -= (f32) delta_y * M_PI / 180.0f / 8.0f;

	input.client->player.pitch = fmax(fmin(input.client->player.pitch, 89.0f), -89.0f);

	set_camera_angle(input.client->player.yaw, input.client->player.pitch);
}

static bool move(int forward, int backward, vec3 dir)
{
	f32 sign;
	f32 speed = 10.0f;

	if (glfwGetKey(input.window, forward) == GLFW_PRESS)
		sign = +1.0f;
	else if (glfwGetKey(input.window, backward) == GLFW_PRESS)
		sign = -1.0f;
	else
		return false;

	input.client->player.velocity.x += dir[0] * speed * sign;
	// input.client->player.velocity.y += dir[1] * speed * sign;
	input.client->player.velocity.z += dir[2] * speed * sign;

	return true;
}

void process_input()
{
	input.client->player.velocity.x = 0.0f;
	// input.client->player.velocity.y = 0.0f;
	input.client->player.velocity.z = 0.0f;

	move(GLFW_KEY_W, GLFW_KEY_S, movement_dirs.front);
	move(GLFW_KEY_SPACE, GLFW_KEY_LEFT_SHIFT, movement_dirs.up);
	move(GLFW_KEY_D, GLFW_KEY_A, movement_dirs.right);
}

void init_input(Client *client, GLFWwindow *window)
{
	input.client = client;
	input.window = window;

	glfwSetCursorPosCallback(input.window, &cursor_pos_callback);
}


