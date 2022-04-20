#include <asprintf/asprintf.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "client/camera.h"
#include "client/client.h"
#include "client/client_config.h"
#include "client/client_inventory.h"
#include "client/client_player.h"
#include "client/debug_menu.h"
#include "client/gui.h"
#include "client/interact.h"
#include "client/input.h"
#include "client/screenshot.h"
#include "client/window.h"
#include "day.h"

#define SET_STATUS_MESSAGE(args...) { \
	char *msg; asprintf(&msg, args); \
	gui_text(status_message, msg); free(msg); \
	status_message->def.text_color.w = 1.01f; }

typedef struct {
	int key;
	bool state;
} KeyListener;

static bool paused = false;

static GUIElement *pause_menu;
static GUIElement *status_message;

static KeyListener listener_pause      = {GLFW_KEY_ESCAPE, false};
static KeyListener listener_fullscreen = {GLFW_KEY_F11,    false};
static KeyListener listener_fly        = {GLFW_KEY_F,      false};
static KeyListener listener_collision  = {GLFW_KEY_C,      false};
static KeyListener listener_timelapse  = {GLFW_KEY_T,      false};
static KeyListener listener_debug_menu = {GLFW_KEY_F3,     false};
static KeyListener listener_screenshot = {GLFW_KEY_F2,     false};

static double cursor_last_x = 0.0;
static double cursor_last_y = 0.0;

// movement mutex needs to be locked
static bool move(int forward, int backward, vec3 dir)
{
	// 25.0f; 4.317f
	f32 sign;

	if (glfwGetKey(window.handle, forward) == GLFW_PRESS)
		sign = +1.0f;
	else if (glfwGetKey(window.handle, backward) == GLFW_PRESS)
		sign = -1.0f;
	else
		return false;

	client_player.velocity.x += dir[0] * client_player.movement.speed * sign;
	client_player.velocity.y += dir[1] * client_player.movement.speed * sign;
	client_player.velocity.z += dir[2] * client_player.movement.speed * sign;

	return true;
}

static void enter_game()
{
	glfwSetInputMode(window.handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	pause_menu->visible = false;
}

static bool key_listener(KeyListener *listener)
{
	bool was = listener->state;
	return !(listener->state = (glfwGetKey(window.handle, listener->key) == GLFW_PRESS)) && was;
}

void input_init()
{
	pause_menu = gui_add(NULL, (GUIElementDef) {
		.pos = {0.0f, 0.0f},
		.z_index = 0.5f,
		.offset = {0, 0},
		.margin = {0, 0},
		.align = {0.0f, 0.0f},
		.scale = {1.0f, 1.0f},
		.scale_type = SCALE_PARENT,
		.affect_parent_scale = false,
		.text = NULL,
		.image = NULL,
		.text_color = {0.0f, 0.0f, 0.0f, 0.0f},
		.bg_color = {0.0f, 0.0f, 0.0f, 0.4f},
	});

	status_message = gui_add(NULL, (GUIElementDef) {
		.pos = {0.5f, 0.25f},
		.z_index = 0.1f,
		.offset = {0, 0},
		.margin = {0, 0},
		.align = {0.5f, 0.5f},
		.scale = {1.0f, 1.0f},
		.scale_type = SCALE_TEXT,
		.affect_parent_scale = false,
		.text = "",
		.image = NULL,
		.text_color = {1.0f, 0.91f, 0.13f, 0.0f},
		.bg_color = {0.0f, 0.0f, 0.0f, 0.0f},
	});

	glfwSetInputMode(window.handle, GLFW_STICKY_KEYS, GL_TRUE);
	enter_game();
}

void input_tick(f64 dtime)
{
	if (status_message->def.text_color.w > 1.0f)
		status_message->def.text_color.w = 1.0f;
	else if (status_message->def.text_color.w > 0.0f)
		status_message->def.text_color.w -= dtime * 1.0f;

	if (key_listener(&listener_pause)) {
		paused = !paused;

		if (paused) {
			glfwSetInputMode(window.handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			pause_menu->visible = true;
		} else {
			enter_game();
		}
	}

	if (key_listener(&listener_fullscreen)) {
		if (window.fullscreen)
			window_exit_fullscreen();
		else
			window_enter_fullscreen();
	}

	if (!paused) {
		if (key_listener(&listener_fly)) {
			pthread_rwlock_wrlock(&client_player.lock_movement);
			client_player.movement.flight = !client_player.movement.flight;

			SET_STATUS_MESSAGE("Flight %s", client_player.movement.flight ? "Enabled" : "Disabled")
			debug_menu_changed(ENTRY_FLIGHT);

			pthread_rwlock_unlock(&client_player.lock_movement);
		}

		if (key_listener(&listener_collision)) {
			pthread_rwlock_wrlock(&client_player.lock_movement);
			client_player.movement.collision = !client_player.movement.collision;

			SET_STATUS_MESSAGE("Collision %s", client_player.movement.collision ? "Enabled" : "Disabled")
			debug_menu_changed(ENTRY_COLLISION);

			pthread_rwlock_unlock(&client_player.lock_movement);
		}

		if (key_listener(&listener_timelapse)) {
			f64 current_time = get_time_of_day();
			timelapse = !timelapse;
			set_time_of_day(current_time);

			SET_STATUS_MESSAGE("Timelapse %s", timelapse ? "Enabled" : "Disabled")
			debug_menu_changed(ENTRY_TIMELAPSE);
		}

		if (key_listener(&listener_debug_menu))
			debug_menu_toggle();

		if (key_listener(&listener_screenshot)) {
			char *screenshot_filename = screenshot();
			SET_STATUS_MESSAGE("Screenshot saved to %s", screenshot_filename)
			free(screenshot_filename);
		}
	}

	pthread_rwlock_rdlock(&client_player.lock_movement);

	client_player.velocity.x = 0.0f;
	client_player.velocity.z = 0.0f;

	if (client_player.movement.flight)
		client_player.velocity.y = 0.0f;

	if (!paused) {
		move(GLFW_KEY_W, GLFW_KEY_S, camera.movement_dirs.front);
		move(GLFW_KEY_D, GLFW_KEY_A, camera.movement_dirs.right);

		if (client_player.movement.flight)
			move(GLFW_KEY_SPACE, GLFW_KEY_LEFT_SHIFT, camera.movement_dirs.up);
		else if (glfwGetKey(window.handle, GLFW_KEY_SPACE) == GLFW_PRESS)
			client_player_jump();
	}

	pthread_rwlock_unlock(&client_player.lock_movement);
}

void input_cursor(double current_x, double current_y)
{
	if (paused)
		return;

	double delta_x = current_x - cursor_last_x;
	double delta_y = current_y - cursor_last_y;
	cursor_last_x = current_x;
	cursor_last_y = current_y;

	ClientEntity *entity = client_player_entity_local();
	if (!entity)
		return;

	pthread_rwlock_wrlock(&entity->lock_pos_rot);

	entity->data.rot.y -= (f32) delta_x * M_PI / 180.0f / 8.0f;
	entity->data.rot.x += (f32) delta_y * M_PI / 180.0f / 8.0f;

	entity->data.rot.y = fmod(entity->data.rot.y + M_PI * 2.0f, M_PI * 2.0f);
	entity->data.rot.x = f32_clamp(entity->data.rot.x, -M_PI / 2.0f + 0.01f, M_PI / 2.0f - 0.01f);

	client_player_update_rot(entity);
	pthread_rwlock_unlock(&entity->lock_pos_rot);
	refcount_drp(&entity->rc);
}

void input_click(bool left)
{
	if (client_config.swap_mouse_buttons)
		left = !left;

	interact_use(left);
}
