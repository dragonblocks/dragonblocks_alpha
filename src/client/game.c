#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <unistd.h>
#include "client/camera.h"
#include "client/client.h"
#include "client/client_entity.h"
#include "client/client_inventory.h"
#include "client/client_item.h"
#include "client/client_node.h"
#include "client/client_player.h"
#include "client/client_terrain.h"
#include "client/debug_menu.h"
#include "client/font.h"
#include "client/frustum.h"
#include "client/game.h"
#include "client/opengl.h"
#include "client/gui.h"
#include "client/input.h"
#include "client/interact.h"
#include "client/sky.h"
#include "client/window.h"
#include "common/day.h"
#include "common/interrupt.h"

#ifdef _WIN32
#include <pthread_time.h>
#endif

int game_fps = 0;

void game_render(f64 dtime)
{
	glEnable(GL_DEPTH_TEST); GL_DEBUG
	glEnable(GL_BLEND); GL_DEBUG
	glEnable(GL_MULTISAMPLE); GL_DEBUG
	glEnable(GL_CULL_FACE); GL_DEBUG

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); GL_DEBUG
	glCullFace(GL_BACK); GL_DEBUG
	glFrontFace(GL_CCW); GL_DEBUG

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); GL_DEBUG

	frustum_update();
	terrain_gfx_update();
	client_entity_gfx_update();
	client_inventory_update(dtime);

	sky_render();
	model_scene_render(dtime);
	interact_render();

	glClear(GL_DEPTH_BUFFER_BIT); GL_DEBUG
	gui_render();
}

static void game_loop()
{
	f64 fps_update_timer = 1.0f;
	unsigned int frames = 0;

	struct timespec ts, ts_old;
	clock_gettime(CLOCK_REALTIME, &ts_old);

	while (!glfwWindowShouldClose(window.handle) && !interrupt.set) {
		clock_gettime(CLOCK_REALTIME, &ts);
		f64 dtime = (f64) (ts.tv_sec - ts_old.tv_sec) + (f64) (ts.tv_nsec - ts_old.tv_nsec) / 1.0e9;
		ts_old = ts;

		if ((fps_update_timer -= dtime) <= 0.0) {
			debug_menu_changed(ENTRY_FPS);
			game_fps = frames;
			fps_update_timer += 1.0;
			frames = 0;
		}

		frames++;

		input_tick(dtime);
		client_player_tick(dtime);
		interact_tick();

		debug_menu_changed(ENTRY_TIME);
		debug_menu_changed(ENTRY_DAYLIGHT);
		debug_menu_changed(ENTRY_SUN_ANGLE);
		debug_menu_update();

		game_render(dtime);

		glfwSwapBuffers(window.handle);
		glfwPollEvents();
	}
}

void game(Flag *gfx_init)
{
	window_init();
	font_init();
	model_init();
	sky_init();
	terrain_gfx_init();
	client_entity_gfx_init();
	client_player_gfx_init();
	camera_init();
	gui_init();
	interact_init();
	client_item_init();
	client_inventory_init();
	client_node_init();
	client_terrain_start();
	debug_menu_init();
	input_init();

	flag_set(gfx_init);
	game_loop();

	client_terrain_stop();
	font_deinit();
	gui_deinit();
	model_deinit();
	sky_deinit();
	terrain_gfx_deinit();
	client_entity_gfx_deinit();
	client_player_gfx_deinit();
	interact_deinit();
	client_item_deinit();
	client_inventory_deinit();
	client_node_deinit();
}

