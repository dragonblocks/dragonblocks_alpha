#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image_write.h>
#include <stdio.h>
#include <unistd.h>
#include "client/camera.h"
#include "client/client.h"
#include "client/client_entity.h"
#include "client/client_node.h"
#include "client/client_player.h"
#include "client/client_terrain.h"
#include "client/debug_menu.h"
#include "client/font.h"
#include "client/frustum.h"
#include "client/game.h"
#include "client/gl_debug.h"
#include "client/gui.h"
#include "client/input.h"
#include "client/interact.h"
#include "client/sky.h"
#include "client/window.h"
#include "day.h"
#include "interrupt.h"

int game_fps = 0;

static void crosshair_init()
{
	gui_add(NULL, (GUIElementDefinition) {
		.pos = {0.5f, 0.5f},
		.z_index = 0.0f,
		.offset = {0, 0},
		.margin = {0, 0},
		.align = {0.5f, 0.5f},
		.scale = {1.0f, 1.0f},
		.scale_type = SCALE_IMAGE,
		.affect_parent_scale = false,
		.text = NULL,
		.image = texture_load(RESSOURCE_PATH "textures/crosshair.png", false),
		.text_color = {0.0f, 0.0f, 0.0f, 0.0f},
		.bg_color = {0.0f, 0.0f, 0.0f, 0.0f},
	});
}

static void render(f64 dtime)
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

	sky_render();
	model_scene_render(dtime);
	interact_render();
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

		render(dtime);

		glfwSwapBuffers(window.handle);
		glfwPollEvents();
	}
}

bool game(Flag *gfx_init)
{
	if (!window_init())
		return false;

	if (!font_init())
		return false;

	model_init();

	if (!sky_init())
		return false;

	if (!terrain_gfx_init())
		return false;

	if (!client_entity_gfx_init())
		return false;

	client_player_gfx_init();

	if (!interact_init())
		return false;

	client_node_init();
	client_terrain_start();

	camera_set_position((v3f32) {0.0f, 0.0f, 0.0f});
	camera_set_angle(0.0f, 0.0f);

	if (!gui_init())
		return false;

	debug_menu_init();
	crosshair_init();
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

	return true;
}

char *game_take_screenshot()
{
	// renderbuffer for depth & stencil buffer
	GLuint rbo;
	glGenRenderbuffers(1, &rbo); GL_DEBUG
	glBindRenderbuffer(GL_RENDERBUFFER, rbo); GL_DEBUG
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 8, GL_DEPTH24_STENCIL8, window.width, window.height); GL_DEBUG

	// 2 textures, one with AA, one without

	GLuint txos[2];
	glGenTextures(2, txos); GL_DEBUG

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, txos[0]); GL_DEBUG
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 8, GL_RGB, window.width, window.height, GL_TRUE); GL_DEBUG

	glBindTexture(GL_TEXTURE_2D, txos[1]); GL_DEBUG
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, window.width, window.height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL); GL_DEBUG

	// 2 framebuffers, one with AA, one without

	GLuint fbos[2];
	glGenFramebuffers(2, fbos); GL_DEBUG

	glBindFramebuffer(GL_FRAMEBUFFER, fbos[0]); GL_DEBUG
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, txos[0], 0); GL_DEBUG
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); GL_DEBUG

	glBindFramebuffer(GL_FRAMEBUFFER, fbos[1]); GL_DEBUG
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, txos[1], 0); GL_DEBUG

	// render scene
	glBindFramebuffer(GL_FRAMEBUFFER, fbos[0]); GL_DEBUG
	render(0.0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0); GL_DEBUG

	// blit AA-buffer into no-AA buffer
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbos[0]); GL_DEBUG
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbos[1]); GL_DEBUG
	glBlitFramebuffer(0, 0, window.width, window.height, 0, 0, window.width, window.height, GL_COLOR_BUFFER_BIT, GL_NEAREST); GL_DEBUG

	// read data
	GLubyte data[window.width * window.height * 3];
	glBindFramebuffer(GL_FRAMEBUFFER, fbos[1]); GL_DEBUG
	glPixelStorei(GL_PACK_ALIGNMENT, 1); GL_DEBUG
	glReadPixels(0, 0, window.width, window.height, GL_RGB, GL_UNSIGNED_BYTE, data); GL_DEBUG

	// create filename
	char filename[BUFSIZ];
	time_t timep = time(0);
	strftime(filename, BUFSIZ, "screenshot-%Y-%m-%d-%H:%M:%S.png", localtime(&timep));

	// save screenshot
	stbi_flip_vertically_on_write(true);
	stbi_write_png(filename, window.width, window.height, 3, data, window.width * 3);

	// delete buffers
	glDeleteRenderbuffers(1, &rbo); GL_DEBUG
	glDeleteTextures(2, txos); GL_DEBUG
	glDeleteFramebuffers(2, fbos); GL_DEBUG

	return strdup(filename);
}
