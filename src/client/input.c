#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "client/action.h"
#include "client/camera.h"
#include "client/client.h"
#include "client/client_config.h"
#include "client/client_inventory.h"
#include "client/client_player.h"
#include "client/gui.h"
#include "client/interact.h"
#include "client/input.h"
#include "client/window.h"

#define NO_AXIS -1, false, 0.0f

typedef struct {
	int key;
	int button;
	int axis;
	bool flip_axis;
	float axis_deadzone;
	int state;
} Trigger;

typedef struct {
	int key_forward;
	int key_backward;
	int button_forward;
	int button_backward;
	int axis;
	bool flip_axis;
} Axis;

static bool paused = false;
static bool inventory = false;
static bool action_menu = false;

static GUIElement *pause_menu;

static Trigger trigger_pause       = { GLFW_KEY_ESCAPE, GLFW_GAMEPAD_BUTTON_B,         NO_AXIS, 0 };
static Trigger trigger_fullscreen  = { GLFW_KEY_F11,    -1,                            NO_AXIS, 0 };
static Trigger trigger_fly         = { GLFW_KEY_F,      -1,                            NO_AXIS, 0 };
static Trigger trigger_collision   = { GLFW_KEY_C,      -1,                            NO_AXIS, 0 };
static Trigger trigger_timelapse   = { GLFW_KEY_T,      -1,                            NO_AXIS, 0 };
static Trigger trigger_debug_menu  = { GLFW_KEY_F3,     -1,                            NO_AXIS, 0 };
static Trigger trigger_screenshot  = { GLFW_KEY_F2,     GLFW_GAMEPAD_BUTTON_BACK,      NO_AXIS, 0 };
static Trigger trigger_inventory   = { GLFW_KEY_I,      GLFW_GAMEPAD_BUTTON_Y,         NO_AXIS, 0 };
static Trigger trigger_action_menu = { GLFW_KEY_DOWN,   GLFW_GAMEPAD_BUTTON_DPAD_DOWN, NO_AXIS, 0 };

static Trigger trigger_use_right = { -1, -1, GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER, false, 0.0f, 0 };
static Trigger trigger_use_left  = { -1, -1, GLFW_GAMEPAD_AXIS_LEFT_TRIGGER,  false, 0.0f, 0 };

static Trigger trigger_menu_up = {
	GLFW_KEY_UP, GLFW_GAMEPAD_BUTTON_DPAD_UP, GLFW_GAMEPAD_AXIS_LEFT_Y, true, 0.5f, 0 };
static Trigger trigger_menu_down = {
	GLFW_KEY_DOWN, GLFW_GAMEPAD_BUTTON_DPAD_DOWN, GLFW_GAMEPAD_AXIS_LEFT_Y, false, 0.5f, 0 };
static Trigger trigger_menu_left = {
	GLFW_KEY_LEFT, GLFW_GAMEPAD_BUTTON_DPAD_LEFT, GLFW_GAMEPAD_AXIS_LEFT_X, true, 0.5f, 0 };
static Trigger trigger_menu_right = {
	GLFW_KEY_RIGHT, GLFW_GAMEPAD_BUTTON_DPAD_RIGHT, GLFW_GAMEPAD_AXIS_LEFT_X, false, 0.5f, 0 };

static Trigger trigger_menu_select = { GLFW_KEY_ENTER, GLFW_GAMEPAD_BUTTON_A, NO_AXIS, 0 };

static Axis axis_front = { GLFW_KEY_W, GLFW_KEY_S, -1, -1, GLFW_GAMEPAD_AXIS_LEFT_Y, true  };
static Axis axis_right = { GLFW_KEY_D, GLFW_KEY_A, -1, -1, GLFW_GAMEPAD_AXIS_LEFT_X, false };

static Axis axis_up = {
	GLFW_KEY_SPACE,                  GLFW_KEY_LEFT_SHIFT,
	GLFW_GAMEPAD_BUTTON_LEFT_BUMPER, GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER, -1, false };

static GLFWgamepadstate gamepad;
static bool gamepad_present;

static double cursor_last_x = 0.0;
static double cursor_last_y = 0.0;

static double cursor_delta_x = 0.0;
static double cursor_delta_y = 0.0;

static int remap_button(int button)
{
	if (client_config.swap_gamepad_buttons) switch (button) {
		case GLFW_GAMEPAD_BUTTON_A: return GLFW_GAMEPAD_BUTTON_B;
		case GLFW_GAMEPAD_BUTTON_B: return GLFW_GAMEPAD_BUTTON_A;
		case GLFW_GAMEPAD_BUTTON_X: return GLFW_GAMEPAD_BUTTON_Y;
		case GLFW_GAMEPAD_BUTTON_Y: return GLFW_GAMEPAD_BUTTON_X;
	}

	return button;
}

static bool get_button(int button)
{
	if (!gamepad_present || button == -1)
		return false;

	return gamepad.buttons[remap_button(button)];
}

static bool get_key(int key)
{
	return key != -1 && glfwGetKey(window.handle, key) == GLFW_PRESS;
}

static float get_axis(int axis)
{
	if (!gamepad_present || axis == -1)
		return 0.0f;

	float value = gamepad.axes[axis];
	if (fabsf(value) < client_config.gamepad_deadzone)
		return 0.0f;

	return value;
}

static bool run_trigger(Trigger *trigger)
{
	bool active = get_key(trigger->key) || get_button(trigger->button) ||
		(trigger->flip_axis ? -1.0 : +1.0) * get_axis(trigger->axis) > trigger->axis_deadzone;

	if (trigger->state % 2 == active)
		trigger->state++;
	if (trigger->state < 3)
		return false;

	trigger->state = 0;
	return true;
}

static void enter_game()
{
	glfwSetInputMode(window.handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	pause_menu->visible = false;
	client_inventory_set_open(inventory = false);
	action_menu_set_open(action_menu = false);
}

static void toggle_pause()
{
	paused = !paused;

	if (paused) {
		glfwSetInputMode(window.handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		pause_menu->visible = true;
	} else {
		enter_game();
	}
}

// movement mutex needs to be locked
static bool move(Axis *axis, vec3 dir)
{
	// 25.0f; 4.317f

	f32 sign = get_axis(axis->axis);
	if (sign != 0.0f && axis->flip_axis)
		sign = -sign;

	if (sign == 0.0f) {
		if (get_key(axis->key_forward) || get_button(axis->button_forward))
			sign = +1.0f;
		else if (get_key(axis->key_backward) || get_button(axis->button_backward))
			sign = -1.0f;
		else
			return false;
	}

	client_player.velocity.x += dir[0] * client_player.movement.speed * sign;
	client_player.velocity.y += dir[1] * client_player.movement.speed * sign;
	client_player.velocity.z += dir[2] * client_player.movement.speed * sign;

	return true;
}

static void update_camera()
{
	ClientEntity *entity = client_player_entity_local();
	if (!entity)
		return;

	double delta_x = cursor_delta_x
		+ get_axis(GLFW_GAMEPAD_AXIS_RIGHT_X) * client_config.gamepad_sensitivity;
	double delta_y = cursor_delta_y
		+ get_axis(GLFW_GAMEPAD_AXIS_RIGHT_Y) * client_config.gamepad_sensitivity;

	pthread_rwlock_wrlock(&entity->lock_pos_rot);

	entity->data.rot.y -= (f32) delta_x * M_PI / 180.0f / 8.0f;
	entity->data.rot.x += (f32) delta_y * M_PI / 180.0f / 8.0f;

	entity->data.rot.y = fmod(entity->data.rot.y + M_PI * 2.0f, M_PI * 2.0f);
	entity->data.rot.x = f32_clamp(entity->data.rot.x, -M_PI / 2.0f + 0.01f, M_PI / 2.0f - 0.01f);

	client_player_update_rot(entity);
	pthread_rwlock_unlock(&entity->lock_pos_rot);
	refcount_drp(&entity->rc);
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

	enter_game();
}

void input_tick(f64 dtime)
{
	if ((gamepad_present = glfwJoystickIsGamepad(GLFW_JOYSTICK_1)))
		glfwGetGamepadState(GLFW_JOYSTICK_1, &gamepad);

	if (run_trigger(&trigger_pause))
		toggle_pause();

	if ((!paused || inventory) && run_trigger(&trigger_inventory)) {
		toggle_pause();
		client_inventory_set_open(inventory = paused);
	}

	if (!paused && run_trigger(&trigger_action_menu)) {
		toggle_pause();
		action_menu_set_open(action_menu = paused);
	}

	if (inventory) {
		if (run_trigger(&trigger_menu_up))
			client_inventory_navigate(MENU_UP);
		if (run_trigger(&trigger_menu_down))
			client_inventory_navigate(MENU_DOWN);
		if (run_trigger(&trigger_menu_left))
			client_inventory_navigate(MENU_LEFT);
		if (run_trigger(&trigger_menu_right))
			client_inventory_navigate(MENU_RIGHT);

		if (run_trigger(&trigger_menu_select))
			client_inventory_select();
	}

	if (action_menu) {
		if (run_trigger(&trigger_menu_up))
			action_menu_navigate(-1);
		if (run_trigger(&trigger_menu_down))
			action_menu_navigate(+1);

		if (run_trigger(&trigger_menu_select)) {
			toggle_pause();
			action_menu_select();
		}
	}

	if (!paused) {
		trigger_menu_up.state = 0;
		trigger_menu_down.state = 0;
		trigger_menu_left.state = 0;
		trigger_menu_right.state = 0;
		trigger_menu_select.state = 0;
	}

	if (run_trigger(&trigger_fullscreen))
		action_toggle_fullscreen();

	if (run_trigger(&trigger_screenshot))
		action_screenshot();

	if (!paused) {
		if (run_trigger(&trigger_use_right))
			interact_use(true);
		if (run_trigger(&trigger_use_left))
			interact_use(false);

		if (run_trigger(&trigger_fly))
			action_toggle_flight();
		if (run_trigger(&trigger_collision))
			action_toggle_collision();
		if (run_trigger(&trigger_timelapse))
			action_quit();
		if (run_trigger(&trigger_debug_menu))
			action_toggle_debug_menu();
	}

	if (!paused)
		update_camera();
	cursor_delta_x = 0.0;
	cursor_delta_y = 0.0;

	pthread_rwlock_rdlock(&client_player.lock_movement);

	client_player.velocity.x = 0.0f;
	client_player.velocity.z = 0.0f;

	if (client_player.movement.flight)
		client_player.velocity.y = 0.0f;

	if (!paused) {
		move(&axis_front, camera.movement_dirs.front);
		move(&axis_right, camera.movement_dirs.right);

		if (client_player.movement.flight)
			move(&axis_up, camera.movement_dirs.up);
		else if (get_key(GLFW_KEY_SPACE) || get_button(GLFW_GAMEPAD_BUTTON_LEFT_THUMB))
			client_player_jump();
	}

	pthread_rwlock_unlock(&client_player.lock_movement);
}

void input_cursor(double current_x, double current_y)
{
	cursor_delta_x = current_x - cursor_last_x;
	cursor_delta_y = current_y - cursor_last_y;
	cursor_last_x = current_x;
	cursor_last_y = current_y;
}

void input_click(bool right)
{
	if (paused) {
		double x, y;
		glfwGetCursorPos(window.handle, &x, &y);
		gui_click(x, y, right);
		return;
	}

	if (client_config.swap_mouse_buttons)
		right = !right;

	interact_use(right);
}

