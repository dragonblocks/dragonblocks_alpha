#include <asprintf/asprintf.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "client/client_config.h"
#include "client/client_player.h"
#include "client/client_terrain.h"
#include "client/debug_menu.h"
#include "client/game.h"
#include "client/gl_debug.h"
#include "client/gui.h"
#include "client/window.h"
#include "day.h"
#include "environment.h"
#include "perlin.h"
#include "version.h"

static GUIElement *gui_elements[COUNT_ENTRY] = {NULL};
static bool changed_elements[COUNT_ENTRY] = {false};
static pthread_mutex_t changed_elements_mtx = PTHREAD_MUTEX_INITIALIZER;

static bool debug_menu_enabled = true;
static DebugMenuEntry last_always_visible = ENTRY_POS;

static char *get_entry_text(DebugMenuEntry entry)
{
	bool flight = false;
	bool collision = false;
	int hours = 0;
	int minutes = 0;
	v3f64 pos = {0.0f, 0.0f, 0.0f};
	v3f32 rot = {0.0f, 0.0f, 0.0f};

	switch (entry) {
		case ENTRY_POS:
		case ENTRY_YAW:
		case ENTRY_PITCH:
		case ENTRY_HUMIDITY:
		case ENTRY_TEMPERATURE: {
			ClientEntity *entity = client_player_entity();
			if (!entity)
				return strdup("");

			pthread_rwlock_rdlock(&entity->lock_pos_rot);
			pos = entity->data.pos;
			rot = entity->data.rot;
			pthread_rwlock_unlock(&entity->lock_pos_rot);
			refcount_drp(&entity->rc);
			break;
		}

		case ENTRY_FLIGHT:
		case ENTRY_COLLISION:
			pthread_rwlock_rdlock(&client_player.lock_movement);
			flight = client_player.movement.flight;
			collision = client_player.movement.collision;
			pthread_rwlock_unlock(&client_player.lock_movement);
			break;

		case ENTRY_ANTIALIASING:
			if (!client_config.antialiasing)
				return strdup("antialiasing: disabled");
			break;

		case ENTRY_TIME:
			split_time_of_day(&hours, &minutes);
			break;

		default:
			break;
	}

	char *str;
	switch (entry) {
		case ENTRY_VERSION:       asprintf(&str, "Dragonblocks Alpha %s", VERSION                                    );          break;
		case ENTRY_FPS:           asprintf(&str, "%d FPS", game_fps                                                  );          break;
		case ENTRY_POS:           asprintf(&str, "(%.1f %.1f %.1f)", pos.x, pos.y, pos.z                             );          break;
		case ENTRY_YAW:           asprintf(&str, "yaw = %.1f", 360.0 - rot.y / M_PI * 180.0                          );          break;
		case ENTRY_PITCH:         asprintf(&str, "pitch = %.1f", -rot.x / M_PI * 180.0                               );          break;
		case ENTRY_TIME:          asprintf(&str, "%02d:%02d", hours, minutes                                         );          break;
		case ENTRY_DAYLIGHT:      asprintf(&str, "daylight = %.2f", get_daylight()                                   );          break;
		case ENTRY_SUN_ANGLE:     asprintf(&str, "sun angle = %.1f", fmod(get_sun_angle() / M_PI * 180.0, 360.0)     );          break;
		case ENTRY_HUMIDITY:      asprintf(&str, "humidity = %.2f", get_humidity((v3s32) {pos.x, pos.y, pos.z})      );          break;
		case ENTRY_TEMPERATURE:   asprintf(&str, "temperature = %.2f", get_temperature((v3s32) {pos.x, pos.y, pos.z}));          break;
		case ENTRY_SEED:          asprintf(&str, "seed = %d", seed                                                   );          break;
		case ENTRY_FLIGHT:        asprintf(&str, "flight: %s", flight ? "enabled" : "disabled"                       );          break;
		case ENTRY_COLLISION:     asprintf(&str, "collision: %s", collision ? "enabled" : "disabled"                 );          break;
		case ENTRY_TIMELAPSE:     asprintf(&str, "timelapse: %s", timelapse ? "enabled" : "disabled"                 );          break;
		case ENTRY_FULLSCREEN:    asprintf(&str, "fullscreen: %s", window.fullscreen ? "enabled" : "disabled"        );          break;
		case ENTRY_OPENGL:        asprintf(&str, "OpenGL %s", glGetString(GL_VERSION)                                ); GL_DEBUG break;
		case ENTRY_GPU:           asprintf(&str, "%s", glGetString(GL_RENDERER)                                      ); GL_DEBUG break;
		case ENTRY_ANTIALIASING:  asprintf(&str, "antialiasing: %u samples", client_config.antialiasing              );          break;
		case ENTRY_MIPMAP:        asprintf(&str, "mipmap: %s", client_config.mipmap ? "enabled" : "disabled"         );          break;
		case ENTRY_VIEW_DISTANCE: asprintf(&str, "view distance: %.1lf", client_config.view_distance                 );          break;
		case ENTRY_LOAD_DISTANCE: asprintf(&str, "load distance: %u", client_terrain_get_load_distance()             );          break;
		default: break;
	}
	return str;
}

void debug_menu_init()
{
	s32 offset = -16;

	for (DebugMenuEntry i = 0; i < COUNT_ENTRY; i++) {
		gui_elements[i] = gui_add(NULL, (GUIElementDefinition) {
			.pos = {0.0f, 0.0f},
			.z_index = 0.1f,
			.offset = {2, offset += 18},
			.margin = {2, 2},
			.align = {0.0f, 0.0f},
			.scale = {1.0f, 1.0f},
			.scale_type = SCALE_TEXT,
			.affect_parent_scale = false,
			.text = "",
			.image = NULL,
			.text_color = (v4f32) {1.0f, 1.0f, 1.0f, 1.0f},
			.bg_color = (v4f32) {0.0f, 0.0f, 0.0f, 0.0f},
		});
	}

	debug_menu_toggle();

	debug_menu_changed(ENTRY_VERSION);
	debug_menu_changed(ENTRY_SEED);
	debug_menu_changed(ENTRY_TIMELAPSE);
	debug_menu_changed(ENTRY_FULLSCREEN);
	debug_menu_changed(ENTRY_OPENGL);
	debug_menu_changed(ENTRY_GPU);
	debug_menu_changed(ENTRY_ANTIALIASING);
	debug_menu_changed(ENTRY_MIPMAP);
	debug_menu_changed(ENTRY_VIEW_DISTANCE);
}

void debug_menu_toggle()
{
	debug_menu_enabled = !debug_menu_enabled;

	for (DebugMenuEntry i = 0; i < COUNT_ENTRY; i++) {
		gui_elements[i]->visible = debug_menu_enabled || i <= last_always_visible;
		gui_elements[i]->def.bg_color.w = debug_menu_enabled ? 0.5f : 0.0f;
	}
}

void debug_menu_update()
{
	bool changed_elements_cpy[COUNT_ENTRY];

	pthread_mutex_lock(&changed_elements_mtx);
	memcpy(changed_elements_cpy, changed_elements, COUNT_ENTRY * sizeof(bool));
	memset(changed_elements, 0,                    COUNT_ENTRY * sizeof(bool));
	pthread_mutex_unlock(&changed_elements_mtx);

	for (DebugMenuEntry i = 0; i < COUNT_ENTRY; i++)
		if (changed_elements_cpy[i]) {
			char *str = get_entry_text(i);
			gui_text(gui_elements[i], str);
			free(str);
		}
}

void debug_menu_changed(DebugMenuEntry entry)
{
	pthread_mutex_lock(&changed_elements_mtx);
	changed_elements[entry] = true;
	pthread_mutex_unlock(&changed_elements_mtx);
}
