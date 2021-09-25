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
#include "client/debug_menu.h"
#include "client/gui.h"
#include "client/input.h"
#include "client/font.h"
#include "client/window.h"
#include "signal_handlers.h"

static void crosshair_init()
{
	gui_add(&gui_root, (GUIElementDefinition) {
		.pos = {0.5f, 0.5f},
		.z_index = 0.0f,
		.offset = {0, 0},
		.margin = {0, 0},
		.align = {0.5f, 0.5f},
		.scale = {1.0f, 1.0f},
		.scale_type = GST_IMAGE,
		.affect_parent_scale = false,
		.text = NULL,
		.image = texture_get(RESSOURCEPATH "textures/crosshair.png"),
		.text_color = (v4f32) {0.0f, 0.0f, 0.0f, 0.0f},
		.bg_color = (v4f32) {0.0f, 0.0f, 0.0f, 0.0f},
	});
}

static void game_loop(Client *client)
{
	f64 fps_update_timer = 1.0f;
	int frames = 0;

	struct timespec ts, ts_old;
	clock_gettime(CLOCK_REALTIME, &ts_old);

	while (! glfwWindowShouldClose(window.handle) && client->state != CS_DISCONNECTED && ! interrupted) {
		clock_gettime(CLOCK_REALTIME, &ts);
		f64 dtime = (f64) (ts.tv_sec - ts_old.tv_sec) + (f64) (ts.tv_nsec - ts_old.tv_nsec) / 1.0e9;
		ts_old = ts;

		if ((fps_update_timer -= dtime) <= 0.0) {
			debug_menu_update_fps(frames);
			fps_update_timer += 1.0;
			frames = 0;
		}

		frames++;

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_ALPHA_TEST);
		glEnable(GL_BLEND);
		glEnable(GL_MULTISAMPLE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glAlphaFunc(GL_GREATER, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.52941176470588f, 0.8078431372549f, 0.92156862745098f, 1.0f);

		input_tick();
		client_player_tick(dtime);

		scene_render();

		glDisable(GL_DEPTH_TEST);
		gui_render();

		glfwSwapBuffers(window.handle);
		glfwPollEvents();
	}
}

bool game(Client *client)
{
	int width, height;
	width = 1250;
	height = 750;

	if (! window_init(width, height))
		return false;

	if (! font_init())
		return false;

	if (! scene_init())
		return false;

	scene_on_resize(width, height);

	client_node_init();
	client_map_start();

	camera_set_position((v3f32) {0.0f, 0.0f, 0.0f});
	camera_set_angle(0.0f, 0.0f);

	if (! gui_init())
		return false;

	gui_on_resize(width, height);

	debug_menu_init();
	debug_menu_toggle();
	debug_menu_update_fps(0);
	debug_menu_update_version();
	debug_menu_update_flight();
	debug_menu_update_collision();
	debug_menu_update_fullscreen();
	debug_menu_update_opengl();
	debug_menu_update_gpu();

	crosshair_init();

	input_init();

	client_player_add_to_scene();

	game_loop(client);

	client_map_stop();

	font_deinit();
	gui_deinit();
	scene_deinit();

	return true;
}
