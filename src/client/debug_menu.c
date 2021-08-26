#include <stdio.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include "environment.h"
#include "client/client_player.h"
#include "client/debug_menu.h"
#include "client/hud.h"
#include "client/window.h"
#include "version.h"

typedef enum
{
	DME_VERSION,
	DME_FPS,
	DME_POS,
	DME_YAW,
	DME_PITCH,
	DME_HUMIDITY,
	DME_TEMPERATURE,
	DME_FLIGHT,
	DME_COLLISION,
	DME_FULLSCREEN,
	DME_OPENGL,
	DME_GPU,
	DME_COUNT,
} DebugMenuEntry;

static HUDElement *huds[DME_COUNT] = {NULL};

static bool debug_menu_enabled = true;
static DebugMenuEntry last_always_visible = DME_POS;

void debug_menu_init()
{
	s32 offset = -16;

	for (DebugMenuEntry i = 0; i < DME_COUNT; i++) {
		huds[i] = hud_add((HUDElementDefinition) {
			.type = HUD_TEXT,
			.pos = {-1.0f, -1.0f, 0.0f},
			.offset = {2, offset += 18},
			.type_def = {
				.text = {
					.text = "",
					.color = {1.0f, 1.0f, 1.0f},
				},
			},
		});
	}
}

void debug_menu_toggle()
{
	debug_menu_enabled = ! debug_menu_enabled;

	for (DebugMenuEntry i = 0; i < DME_COUNT; i++)
		huds[i]->visible = debug_menu_enabled || i <= last_always_visible;
}

void debug_menu_update_version()
{
	char text[BUFSIZ];
	sprintf(text, "Dragonblocks Alpha %s", VERSION);
	hud_change_text(huds[DME_VERSION], text);
}

void debug_menu_update_fps(int fps)
{
	char text[BUFSIZ];
	sprintf(text, "%d FPS", fps);
	hud_change_text(huds[DME_FPS], text);
}

void debug_menu_update_pos()
{
	char text[BUFSIZ];
	sprintf(text, "(%.1f %.1f %.1f)", client_player.pos.x, client_player.pos.y, client_player.pos.z);
	hud_change_text(huds[DME_POS], text);
}

void debug_menu_update_yaw()
{
	char text[BUFSIZ];
	sprintf(text, "yaw = %.1f", client_player.yaw / M_PI * 180.0);
	hud_change_text(huds[DME_YAW], text);
}

void debug_menu_update_pitch()
{
	char text[BUFSIZ];
	sprintf(text, "pitch = %.1f", client_player.pitch / M_PI * 180.0);
	hud_change_text(huds[DME_PITCH], text);
}

void debug_menu_update_humidity()
{
	char text[BUFSIZ];
	sprintf(text, "humidity = %.2f", get_humidity((v3s32) {client_player.pos.x, client_player.pos.y, client_player.pos.z}));
	hud_change_text(huds[DME_HUMIDITY], text);
}

void debug_menu_update_temperature()
{
	char text[BUFSIZ];
	sprintf(text, "temperature = %.2f", get_temperature((v3s32) {client_player.pos.x, client_player.pos.y, client_player.pos.z}));
	hud_change_text(huds[DME_TEMPERATURE], text);
}

void debug_menu_update_flight()
{
	char text[BUFSIZ];
	sprintf(text, "flight: %s", client_player.fly ? "enabled" : "disabled");
	hud_change_text(huds[DME_FLIGHT], text);
}

void debug_menu_update_collision()
{
	char text[BUFSIZ];
	sprintf(text, "collision: %s", client_player.collision ? "enabled" : "disabled");
	hud_change_text(huds[DME_COLLISION], text);
}

void debug_menu_update_fullscreen()
{
	char text[BUFSIZ];
	sprintf(text, "fullscreen: %s", window.fullscreen ? "enabled" : "disabled");
	hud_change_text(huds[DME_FULLSCREEN], text);
}

void debug_menu_update_opengl()
{
	char text[BUFSIZ];
	sprintf(text, "OpenGL %s", glGetString(GL_VERSION));
	hud_change_text(huds[DME_OPENGL], text);
}

void debug_menu_update_gpu()
{
	char text[BUFSIZ];
	sprintf(text, "%s", glGetString(GL_RENDERER));
	hud_change_text(huds[DME_GPU], text);
}
