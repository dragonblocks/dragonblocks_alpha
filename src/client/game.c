#include <stdio.h>
#include <unistd.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include "client/camera.h"
#include "client/client.h"
#include "client/client_map.h"
#include "client/client_node.h"
#include "client/client_player.h"
#include "client/hud.h"
#include "client/input.h"
#include "client/font.h"
#include "client/window.h"
#include "signal.h"

static void game_loop(Client *client)
{
	HUDElement *fps_hud = hud_add((HUDElementDefinition) {
		.type = HUD_TEXT,
		.pos = {-1.0f, -1.0f, 0.0f},
		.offset = {5, 32},
		.type_def = {
			.text = {
				.text = "",
				.color = {1.0f, 1.0f, 1.0f},
			},
		},
	});

	f64 fps_update_timer = 1.0f;
	int frames = 0;

	struct timespec ts, ts_old;
	clock_gettime(CLOCK_REALTIME, &ts_old);

	while (! glfwWindowShouldClose(window.handle) && client->state != CS_DISCONNECTED && ! interrupted) {
		clock_gettime(CLOCK_REALTIME, &ts);
		f64 dtime = (f64) (ts.tv_sec - ts_old.tv_sec) + (f64) (ts.tv_nsec - ts_old.tv_nsec) / 1000000000.0;
		ts_old = ts;

		if ((fps_update_timer -= dtime) <= 0.0) {
			// I don't think anyone will have more than 9999 FPS lol, but I have seen 1000 in jordan4ibanez' video yet so...
			// 4 digits for FPS + space + "FPS" + \0
			char fps[4 + 1 + 3 + 1];
			sprintf(fps, "%d FPS", frames);
			hud_change_text(fps_hud, fps);
			fps_update_timer += 1.0;
			frames = 0;
		}

		frames++;

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glEnable(GL_MULTISAMPLE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.52941176470588f, 0.8078431372549f, 0.92156862745098f, 1.0f);

		input_tick();
		client_player_tick(dtime);

		scene_render();
		hud_render();
		// font_render();

		glfwSwapBuffers(window.handle);
		glfwPollEvents();
	}
}

void game(Client *client)
{
	int width, height;
	width = 1250;
	height = 750;

	if (! window_init(width, height))
		return;

	if (! font_init())
		return;

	if (! scene_init())
		return;

	scene_on_resize(width, height);

	client_node_init();
	client_map_start_meshgen();

	camera_set_position((v3f) {0.0f, 0.0f, 0.0f});
	camera_set_angle(0.0f, 0.0f);

	hud_init();
	hud_on_resize(width, height);

	hud_add((HUDElementDefinition) {
		.type = HUD_IMAGE,
		.pos = {0.0f, 0.0f, 0.0f},
		.offset = {0, 0},
		.type_def = {
			.image = {
				.texture = texture_get(RESSOURCEPATH "textures/crosshair.png"),
				.scale = {1.0f, 1.0f},
				.scale_type = HUD_SCALE_TEXTURE,
			},
		},
	});

	hud_add((HUDElementDefinition) {
		.type = HUD_TEXT,
		.pos = {-1.0f, -1.0f, 0.0f},
		.offset = {5, 0},
		.type_def = {
			.text = {
				.text = "Dragonblocks Alpha",
				.color = {1.0f, 1.0f, 1.0f},
			},
		},
	});

	input_init();

	client_player_add_to_scene();

	game_loop(client);

	font_deinit();
	hud_deinit();
	scene_deinit();
}
