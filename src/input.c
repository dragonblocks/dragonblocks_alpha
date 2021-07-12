#include <math.h>
#include "camera.h"
#include "client.h"
#include "hud.h"
#include "input.h"

typedef struct
{
	int key;
	bool was_pressed;
	bool fired;
} KeyListener;

static struct
{
	GLFWwindow *window;
	Client *client;
	HUDElement *pause_menu_hud;
	bool paused;
	bool fullscreen;
	KeyListener pause_listener;
	KeyListener fullscreen_listener;
	int small_x, small_y, small_width, small_height;
} input;

void input_on_cursor_pos(double current_x, double current_y)
{
	if (input.paused)
		return;

	static double last_x, last_y = 0.0;

	double delta_x = current_x - last_x;
	double delta_y = current_y - last_y;
	last_x = current_x;
	last_y = current_y;

	input.client->player.yaw += (f32) delta_x * M_PI / 180.0f / 8.0f;
	input.client->player.pitch -= (f32) delta_y * M_PI / 180.0f / 8.0f;

	input.client->player.pitch = fmax(fmin(input.client->player.pitch, M_PI / 2.0f - 0.01f), -M_PI / 2.0f + 0.01f);

	set_camera_angle(input.client->player.yaw, input.client->player.pitch);
}

void input_on_resize(int width, int height)
{
	if (! input.fullscreen) {
		input.small_width = width;
		input.small_height = height;
	}
}

void input_on_window_pos(int x, int y)
{
	if (! input.fullscreen) {
		input.small_x = x;
		input.small_y = y;
	}
}

static bool move(int forward, int backward, vec3 dir)
{
	f32 sign;
	f32 speed = 4.317f;

	if (glfwGetKey(input.window, forward) == GLFW_PRESS)
		sign = +1.0f;
	else if (glfwGetKey(input.window, backward) == GLFW_PRESS)
		sign = -1.0f;
	else
		return false;

	input.client->player.velocity.x += dir[0] * speed * sign;
	input.client->player.velocity.z += dir[2] * speed * sign;

	return true;
}

static void enter_game()
{
	glfwSetInputMode(input.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	input.pause_menu_hud->visible = false;
}

static void do_key_listener(KeyListener *listener)
{
	bool is_pressed = glfwGetKey(input.window, listener->key) == GLFW_PRESS;
	listener->fired = listener->was_pressed && ! is_pressed;
	listener->was_pressed = is_pressed;
}

static KeyListener create_key_listener(int key)
{
	return (KeyListener) {
		.key = key,
		.was_pressed = false,
		.fired = false,
	};
}

void process_input()
{
	do_key_listener(&input.pause_listener);
	do_key_listener(&input.fullscreen_listener);

	if (input.pause_listener.fired) {
		input.paused = ! input.paused;

		if (input.paused) {
			glfwSetInputMode(input.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			input.pause_menu_hud->visible = true;
		} else {
			enter_game();
		}
	}

	if (input.fullscreen_listener.fired) {
		input.fullscreen = ! input.fullscreen;

		if (input.fullscreen) {
			GLFWmonitor *monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode *vidmode = glfwGetVideoMode(monitor);
			glfwSetWindowMonitor(input.window, monitor, 0, 0, vidmode->width, vidmode->height, 0);
		} else {
			glfwSetWindowMonitor(input.window, NULL, input.small_x, input.small_y, input.small_width, input.small_height, 0);
		}
	}

	input.client->player.velocity.x = 0.0f;
	input.client->player.velocity.z = 0.0f;

	if (! input.paused) {
		move(GLFW_KEY_W, GLFW_KEY_S, movement_dirs.front);
		move(GLFW_KEY_D, GLFW_KEY_A, movement_dirs.right);

		if (glfwGetKey(input.window, GLFW_KEY_SPACE) == GLFW_PRESS)
			clientplayer_jump(&input.client->player);
	}
}

void init_input(Client *client, GLFWwindow *window)
{
	input.client = client;
	input.window = window;

	input.paused = false;

	input.pause_listener = create_key_listener(GLFW_KEY_ESCAPE);
	input.fullscreen_listener = create_key_listener(GLFW_KEY_F11);

	input.pause_menu_hud = hud_add(RESSOURCEPATH "textures/pause_layer.png", (v3f) {-1.0f, -1.0f, 0.5f}, (v2f) {1.0f, 1.0f}, HUD_SCALE_SCREEN);

	enter_game();
}


