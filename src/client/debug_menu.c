#include <stdio.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include "client/client_config.h"
#include "client/client_map.h"
#include "client/client_player.h"
#include "client/debug_menu.h"
#include "client/gui.h"
#include "client/window.h"
#include "day.h"
#include "environment.h"
#include "perlin.h"
#include "util.h"
#include "version.h"

typedef enum
{
	DME_VERSION,
	DME_FPS,
	DME_POS,
	DME_YAW,
	DME_PITCH,
	DME_TIME,
	DME_DAYLIGHT,
	DME_SUN_ANGLE,
	DME_HUMIDITY,
	DME_TEMPERATURE,
	DME_SEED,
	DME_FLIGHT,
	DME_COLLISION,
	DME_TIMELAPSE,
	DME_FULLSCREEN,
	DME_OPENGL,
	DME_GPU,
	DME_ANTIALIASING,
	DME_MIPMAP,
	DME_RENDER_DISTANCE,
	DME_SIMULATION_DISTANCE,
	DME_COUNT,
} DebugMenuEntry;

static GUIElement *gui_elements[DME_COUNT] = {NULL};

static bool debug_menu_enabled = true;
static DebugMenuEntry last_always_visible = DME_POS;

void debug_menu_init()
{
	s32 offset = -16;

	for (DebugMenuEntry i = 0; i < DME_COUNT; i++) {
		gui_elements[i] = gui_add(&gui_root, (GUIElementDefinition) {
			.pos = {0.0f, 0.0f},
			.z_index = 0.1f,
			.offset = {2, offset += 18},
			.margin = {2, 2},
			.align = {0.0f, 0.0f},
			.scale = {1.0f, 1.0f},
			.scale_type = GST_TEXT,
			.affect_parent_scale = false,
			.text = strdup(""),
			.image = NULL,
			.text_color = (v4f32) {1.0f, 1.0f, 1.0f, 1.0f},
			.bg_color = (v4f32) {0.0f, 0.0f, 0.0f, 0.0f},
		});
	}
}

void debug_menu_toggle()
{
	debug_menu_enabled = ! debug_menu_enabled;

	for (DebugMenuEntry i = 0; i < DME_COUNT; i++) {
		gui_elements[i]->visible = debug_menu_enabled || i <= last_always_visible;
		gui_elements[i]->def.bg_color.w = debug_menu_enabled ? 0.5f : 0.0f;
	}
}

void debug_menu_update_version()
{
	gui_set_text(gui_elements[DME_VERSION], format_string("Dragonblocks Alpha %s", VERSION));
}

void debug_menu_update_fps(int fps)
{
	gui_set_text(gui_elements[DME_FPS], format_string("%d FPS", fps));
}

void debug_menu_update_pos()
{
	gui_set_text(gui_elements[DME_POS], format_string("(%.1f %.1f %.1f)", client_player.pos.x, client_player.pos.y, client_player.pos.z));
}

void debug_menu_update_yaw()
{
	gui_set_text(gui_elements[DME_YAW], format_string("yaw = %.1f", client_player.yaw / M_PI * 180.0));
}

void debug_menu_update_pitch()
{
	gui_set_text(gui_elements[DME_PITCH], format_string("pitch = %.1f", client_player.pitch / M_PI * 180.0));
}

void debug_menu_update_time()
{
	int hours, minutes;
	split_time_of_day(&hours, &minutes);
	gui_set_text(gui_elements[DME_TIME], format_string("%02d:%02d", hours, minutes));
}

void debug_menu_update_daylight()
{
	gui_set_text(gui_elements[DME_DAYLIGHT], format_string("daylight = %.2f", get_daylight()));
}

void debug_menu_update_sun_angle()
{
	gui_set_text(gui_elements[DME_SUN_ANGLE], format_string("sun angle = %.1f", fmod(get_sun_angle() / M_PI * 180.0, 360.0)));
}

void debug_menu_update_humidity()
{
	gui_set_text(gui_elements[DME_HUMIDITY], format_string("humidity = %.2f", get_humidity((v3s32) {client_player.pos.x, client_player.pos.y, client_player.pos.z})));
}

void debug_menu_update_temperature()
{
	gui_set_text(gui_elements[DME_TEMPERATURE], format_string("temperature = %.2f", get_temperature((v3s32) {client_player.pos.x, client_player.pos.y, client_player.pos.z})));
}

void debug_menu_update_seed()
{
	gui_set_text(gui_elements[DME_SEED], format_string("seed = %d", seed));
}

void debug_menu_update_flight()
{
	gui_set_text(gui_elements[DME_FLIGHT], format_string("flight: %s", client_player.fly ? "enabled" : "disabled"));
}

void debug_menu_update_collision()
{
	gui_set_text(gui_elements[DME_COLLISION], format_string("collision: %s", client_player.collision ? "enabled" : "disabled"));
}

void debug_menu_update_timelapse()
{
	gui_set_text(gui_elements[DME_TIMELAPSE], format_string("timelapse: %s", timelapse ? "enabled" : "disabled"));
}

void debug_menu_update_fullscreen()
{
	gui_set_text(gui_elements[DME_FULLSCREEN], format_string("fullscreen: %s", window.fullscreen ? "enabled" : "disabled"));
}

void debug_menu_update_opengl()
{
	gui_set_text(gui_elements[DME_OPENGL], format_string("OpenGL %s", glGetString(GL_VERSION)));
}

void debug_menu_update_gpu()
{
	gui_set_text(gui_elements[DME_GPU], format_string("%s", glGetString(GL_RENDERER)));
}

void debug_menu_update_antialiasing()
{
	gui_set_text(gui_elements[DME_ANTIALIASING], client_config.antialiasing > 1
		? format_string("antialiasing: %u samples", client_config.antialiasing)
		: format_string("antialiasing: disabled")
	);
}

void debug_menu_update_mipmap()
{
	gui_set_text(gui_elements[DME_MIPMAP], format_string("mipmap: %s", client_config.mipmap ? "enabled" : "disabled"));
}

void debug_menu_update_render_distance()
{
	gui_set_text(gui_elements[DME_RENDER_DISTANCE], format_string("render distance: %.1lf", client_config.render_distance));
}

void debug_menu_update_simulation_distance()
{
	gui_set_text(gui_elements[DME_SIMULATION_DISTANCE], format_string("simulation distance: %u", client_map.simulation_distance));
}
