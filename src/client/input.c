#include <math.h>
#include "client/camera.h"
#include "client/client.h"
#include "client/client_player.h"
#include "client/debug_menu.h"
#include "client/gui.h"
#include "client/input.h"
#include "client/window.h"
#include "day.h"

typedef struct
{
	int key;
	bool was_pressed;
	bool fired;
} KeyListener;

static struct
{
	GUIElement *pause_menu;
	bool paused;
	KeyListener pause_listener;
	KeyListener fullscreen_listener;
	KeyListener fly_listener;
	KeyListener collision_listener;
	KeyListener timelapse_listener;
	KeyListener debug_menu_listener;
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

	client_player.yaw += (f32) delta_x * M_PI / 180.0f / 8.0f;
	client_player.pitch -= (f32) delta_y * M_PI / 180.0f / 8.0f;

	client_player.yaw = fmod(client_player.yaw + M_PI * 2.0f, M_PI * 2.0f);
	client_player.pitch = fmax(fmin(client_player.pitch, M_PI / 2.0f - 0.01f), -M_PI / 2.0f + 0.01f);

	camera_set_angle(client_player.yaw, client_player.pitch);

	debug_menu_update_yaw();
	debug_menu_update_pitch();
}

static bool move(int forward, int backward, vec3 dir)
{
	f64 sign;
	f64 speed = client_player.fly ? 25.0f : 4.317f;

	if (glfwGetKey(window.handle, forward) == GLFW_PRESS)
		sign = +1.0f;
	else if (glfwGetKey(window.handle, backward) == GLFW_PRESS)
		sign = -1.0f;
	else
		return false;

	client_player.velocity.x += dir[0] * speed * sign;
	client_player.velocity.y += dir[1] * speed * sign;
	client_player.velocity.z += dir[2] * speed * sign;

	return true;
}

static void enter_game()
{
	glfwSetInputMode(window.handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	input.pause_menu->visible = false;
}

static void do_key_listener(KeyListener *listener)
{
	bool is_pressed = glfwGetKey(window.handle, listener->key) == GLFW_PRESS;
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

void input_tick()
{
	do_key_listener(&input.pause_listener);
	do_key_listener(&input.fullscreen_listener);

	if (input.pause_listener.fired) {
		input.paused = ! input.paused;

		if (input.paused) {
			glfwSetInputMode(window.handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			input.pause_menu->visible = true;
		} else {
			enter_game();
		}
	}

	if (input.fullscreen_listener.fired) {
		if (window.fullscreen)
			window_exit_fullscreen();
		else
			window_enter_fullscreen();
	}

	if (! input.paused) {
		do_key_listener(&input.fly_listener);
		do_key_listener(&input.collision_listener);
		do_key_listener(&input.timelapse_listener);
		do_key_listener(&input.debug_menu_listener);

		if (input.fly_listener.fired) {
			client_player.fly = ! client_player.fly;
			debug_menu_update_flight();
		}

		if (input.collision_listener.fired) {
			client_player.collision = ! client_player.collision;
			debug_menu_update_collision();
		}

		if (input.timelapse_listener.fired) {
			f64 current_time = get_time_of_day();
			timelapse = ! timelapse;
			set_time_of_day(current_time);
			debug_menu_update_timelapse();
		}

		if (input.debug_menu_listener.fired)
			debug_menu_toggle();
	}

	client_player.velocity.x = 0.0f;
	client_player.velocity.z = 0.0f;

	if (client_player.fly)
		client_player.velocity.y = 0.0f;

	if (! input.paused) {
		move(GLFW_KEY_W, GLFW_KEY_S, camera.movement_dirs.front);
		move(GLFW_KEY_D, GLFW_KEY_A, camera.movement_dirs.right);

		if (client_player.fly)
			move(GLFW_KEY_SPACE, GLFW_KEY_LEFT_SHIFT, camera.movement_dirs.up);
		else if (glfwGetKey(window.handle, GLFW_KEY_SPACE) == GLFW_PRESS)
			client_player_jump();
	}
}

void input_init()
{
	input.paused = false;

	input.pause_listener = create_key_listener(GLFW_KEY_ESCAPE);
	input.fullscreen_listener = create_key_listener(GLFW_KEY_F11);
	input.fly_listener = create_key_listener(GLFW_KEY_F);
	input.collision_listener = create_key_listener(GLFW_KEY_C);
	input.timelapse_listener = create_key_listener(GLFW_KEY_T);
	input.debug_menu_listener = create_key_listener(GLFW_KEY_F3);

	input.pause_menu = gui_add(&gui_root, (GUIElementDefinition) {
		.pos = {0.0f, 0.0f},
		.z_index = 0.5f,
		.offset = {0, 0},
		.margin = {0, 0},
		.align = {0.0f, 0.0f},
		.scale = {1.0f, 1.0f},
		.scale_type = GST_PARENT,
		.affect_parent_scale = false,
		.text = NULL,
		.image = NULL,
		.text_color = {0.0f, 0.0f, 0.0f, 0.0f},
		.bg_color = {0.0f, 0.0f, 0.0f, 0.4f},
	});

	glfwSetInputMode(window.handle, GLFW_STICKY_KEYS, GL_TRUE);

	enter_game();
}
