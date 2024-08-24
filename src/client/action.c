#include <stdio.h>
#include <stdlib.h>
#include "client/action.h"
#include "client/client_player.h"
#include "client/debug_menu.h"
#include "client/gui.h"
#include "client/screenshot.h"
#include "client/window.h"
#include "common/color.h"
#include "common/day.h"
#include "common/interrupt.h"

#define SET_STATUS_MESSAGE(args...) { \
	char *msg; asprintf(&msg, args); \
	gui_text(status_message, msg); free(msg); \
	status_message->def.text_color.w = 1.01f; }

static GUIElement *status_message;
static GUIElement *action_menu;

#define ACTION_TEX ASSET_PATH "textures/actions/"

struct {
	char *text;
	void (*action)();
	GUIElement *gui;
	char *icon;
} menu_entries[] = {
	{ "Take Screenshot",   &action_screenshot,        NULL, ACTION_TEX "screenshot.png" },
	{ "Toggle Fullscreen", &action_toggle_fullscreen, NULL, ACTION_TEX "fullscreen.png" },
	{ "Toggle Debug Menu", &action_toggle_debug_menu, NULL, ACTION_TEX "debug_menu.png" },
	{ "Toggle Flight",     &action_toggle_flight,     NULL, ACTION_TEX "flight.png"     },
	{ "Toggle Timelapse",  &action_toggle_timelapse,  NULL, ACTION_TEX "timelapse.png"  },
	{ "Toggle Collision",  &action_toggle_collision,  NULL, ACTION_TEX "collision.png"  },
	{ "Quit Game",         &action_quit,              NULL, ACTION_TEX "quit.png"       },
};

static const size_t num_entries = sizeof menu_entries / sizeof *menu_entries;
static size_t selected_entry = 0;

static void select_entry(size_t entry)
{
	menu_entries[selected_entry].gui->def.bg_color = color_from_u32(0);
	menu_entries[selected_entry = entry].gui->def.bg_color = color_from_u32(0xff0aa965);
}

void action_init()
{
	status_message = gui_add(NULL, (GUIElementDef) {
		.pos = {0.5f, 0.25f},
		.z_index = 0.1f,
		.align = {0.5f, 0.5f},
		.scale = {1.0f, 1.0f},
		.scale_type = SCALE_TEXT,
		.text = "",
		.text_color = {1.0f, 0.91f, 0.13f, 0.0f},
	});

	const float aspect_ratio = 0.75;

	action_menu = gui_add(NULL, (GUIElementDef) {
		.pos = { 0.5, 0.5 },
		.z_index = 0.6,
		.scale = { 0.85, 0.85 },
		.align = { 0.5, 0.5 },
		.scale_type = SCALE_RATIO,
		.ratio = aspect_ratio,
		.bg_color = color_from_u32(0xff08653d),
	});
	action_menu->visible = false;

	float entry_height = 1.0/num_entries;
	for (size_t i = 0; i < num_entries; i++) {
		menu_entries[i].gui = gui_add(action_menu, (GUIElementDef) {
			.pos = { 0.0, entry_height * i },
			.scale = { 1.0, entry_height },
			.scale_type = SCALE_PARENT,
		});

		GUIElement *image_container = gui_add(menu_entries[i].gui, (GUIElementDef) {
			.pos = { 0.0, 0.0 },
			.scale = { 1.0, 1.0 },
			.scale_type = SCALE_RATIO,
			.ratio = 1.0,
		});

		gui_add(image_container, (GUIElementDef) {
			.pos = { 0.5, 0.5 },
			.align = { 0.5, 0.5 },
			.scale = { 0.8, 0.8 },
			.scale_type = SCALE_PARENT,
			.image = texture_load(menu_entries[i].icon, false),
		});

		gui_add(menu_entries[i].gui, (GUIElementDef) {
			.pos = { entry_height / aspect_ratio, 0.5 },
			.align = { 0.0, 0.5 },
			.scale = { 1.0, 1.0 },
			.scale_type = SCALE_TEXT,
			.text = menu_entries[i].text,
			.text_color = color_from_u32(0xffdddddd),
		});
	}
}

void action_tick(f64 dtime)
{
	if (status_message->def.text_color.w > 1.0f)
		status_message->def.text_color.w = 1.0f;
	else if (status_message->def.text_color.w > 0.0f)
		status_message->def.text_color.w -= dtime * 1.0f;
}

void action_menu_navigate(int dir)
{
	select_entry((selected_entry + num_entries + dir) % num_entries);
}

void action_menu_set_open(bool open)
{
	if (open) select_entry(selected_entry);
	action_menu->visible = open;
}

void action_menu_select()
{
	menu_entries[selected_entry].action();
}

void action_screenshot()
{
	char *screenshot_filename = screenshot();
	SET_STATUS_MESSAGE("Screenshot saved to %s", screenshot_filename)
	free(screenshot_filename);
}

void action_toggle_fullscreen()
{
	if (window.fullscreen)
		window_exit_fullscreen();
	else
		window_enter_fullscreen();
}

void action_toggle_flight()
{
	pthread_rwlock_wrlock(&client_player.lock_movement);
	client_player.movement.flight = !client_player.movement.flight;

	SET_STATUS_MESSAGE("Flight %s", client_player.movement.flight ? "Enabled" : "Disabled")
	debug_menu_changed(ENTRY_FLIGHT);

	pthread_rwlock_unlock(&client_player.lock_movement);
}

void action_toggle_debug_menu()
{
	debug_menu_toggle();
}

void action_toggle_timelapse()
{
	f64 current_time = get_time_of_day();
	timelapse = !timelapse;
	set_time_of_day(current_time);

	SET_STATUS_MESSAGE("Timelapse %s", timelapse ? "Enabled" : "Disabled")
	debug_menu_changed(ENTRY_TIMELAPSE);
}

void action_toggle_collision()
{
	pthread_rwlock_wrlock(&client_player.lock_movement);
	client_player.movement.collision = !client_player.movement.collision;

	SET_STATUS_MESSAGE("Collision %s", client_player.movement.collision ? "Enabled" : "Disabled")
	debug_menu_changed(ENTRY_COLLISION);

	pthread_rwlock_unlock(&client_player.lock_movement);
}

void action_quit()
{
	flag_set(&interrupt);
}
