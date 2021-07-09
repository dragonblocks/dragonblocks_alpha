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

	input.client->yaw += (float) delta_x * M_PI / 180.0f / 8.0f;
	input.client->pitch -= (float) delta_y * M_PI / 180.0f / 8.0f;

	input.client->pitch = fmax(fmin(input.client->pitch, 90.0f), -90.0f);

	set_camera_angle(input.client->yaw, input.client->pitch);
}

static bool move(int forward, int backward, vec3 speed)
{
	float sign;

	if (glfwGetKey(input.window, forward) == GLFW_PRESS)
		sign = +1.0f;
	else if (glfwGetKey(input.window, backward) == GLFW_PRESS)
		sign = -1.0f;
	else
		return false;

	input.client->pos.x += speed[0] * sign;
	input.client->pos.y += speed[1] * sign;
	input.client->pos.z += speed[2] * sign;

	return true;
}

void process_input()
{
	bool moved_forward = move(GLFW_KEY_W, GLFW_KEY_S, movement_dirs.front);
	bool moved_up = move(GLFW_KEY_SPACE, GLFW_KEY_LEFT_SHIFT, movement_dirs.up);
	bool moved_right = move(GLFW_KEY_D, GLFW_KEY_A, movement_dirs.right);

	if (moved_forward || moved_up || moved_right) {
		set_camera_position(input.client->pos);

		pthread_mutex_lock(&input.client->mtx);
		(void) (write_u32(input.client->fd, SC_POS) && write_v3f32(input.client->fd, input.client->pos));
		pthread_mutex_unlock(&input.client->mtx);
	}
}

void init_input(Client *client, GLFWwindow *window)
{
	input.client = client;
	input.window = window;

	glfwSetCursorPosCallback(input.window, &cursor_pos_callback);
}


