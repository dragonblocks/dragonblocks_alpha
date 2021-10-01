#include <stdio.h>
#include <unistd.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#include "client/camera.h"
#include "client/client.h"
#include "client/client_map.h"
#include "client/client_node.h"
#include "client/client_player.h"
#include "client/debug_menu.h"
#include "client/font.h"
#include "client/gui.h"
#include "client/input.h"
#include "client/sky.h"
#include "client/window.h"
#include "day.h"
#include "signal_handlers.h"

int window_width, window_height;

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

static void render(f64 dtime)
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_CULL_FACE);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glAlphaFunc(GL_GREATER, 0.0f);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	sky_render();
	scene_render(dtime);
	gui_render();
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

		input_tick(dtime);
		client_player_tick(dtime);

		debug_menu_update_time();
		debug_menu_update_daylight();
		debug_menu_update_sun_angle();

		render(dtime);

		glfwSwapBuffers(window.handle);
		glfwPollEvents();
	}
}

bool game(Client *client)
{
	window_width = 1250;
	window_height = 750;

	if (! window_init(window_width, window_height))
		return false;

	if (! font_init())
		return false;

	if (! scene_init())
		return false;

	scene_on_resize(window_width, window_height);

	if (! sky_init())
		return false;

	client_node_init();
	client_map_start();

	camera_set_position((v3f32) {0.0f, 0.0f, 0.0f});
	camera_set_angle(0.0f, 0.0f);

	if (! gui_init())
		return false;

	gui_on_resize(window_width, window_height);

	debug_menu_init();
	debug_menu_toggle();
	debug_menu_update_fps(0);
	debug_menu_update_version();
	debug_menu_update_seed();
	debug_menu_update_flight();
	debug_menu_update_collision();
	debug_menu_update_timelapse();
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
	sky_deinit();

	return true;
}

char *take_screenshot()
{
	// renderbuffer for depth & stencil buffer
	GLuint RBO;
	glGenRenderbuffers(1, &RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, RBO);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 8, GL_DEPTH24_STENCIL8, window_width, window_height);

	// 2 textures, one with AA, one without

	GLuint textures[2];
	glGenTextures(2, textures);

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textures[0]);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 8, GL_RGB, window_width, window_height, GL_TRUE);

	glBindTexture(GL_TEXTURE_2D, textures[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, window_width, window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	// 2 framebuffers, one with AA, one without

	GLuint FBOs[2];
	glGenFramebuffers(2, FBOs);

	glBindFramebuffer(GL_FRAMEBUFFER, FBOs[0]);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, textures[0], 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

	glBindFramebuffer(GL_FRAMEBUFFER, FBOs[1]);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[1], 0);

	// render scene
	glBindFramebuffer(GL_FRAMEBUFFER, FBOs[0]);
	render(0.0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// blit AA-buffer into no-AA buffer
	glBindFramebuffer(GL_READ_FRAMEBUFFER, FBOs[0]);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBOs[1]);
	glBlitFramebuffer(0, 0, window_width, window_height, 0, 0, window_width, window_height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	// read data
	GLubyte data[window_width * window_height * 3];
	glBindFramebuffer(GL_FRAMEBUFFER, FBOs[1]);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, window_width, window_height, GL_RGB, GL_UNSIGNED_BYTE, data);

	// create filename
	char filename[BUFSIZ];
	time_t timep = time(0);
	strftime(filename, BUFSIZ, "screenshot-%Y-%m-%d-%H:%M:%S.png", localtime(&timep));

	// save screenshot
	stbi_flip_vertically_on_write(true);
	stbi_write_png(filename, window_width, window_height, 3, data, window_width * 3);

	// delete buffers
	glDeleteRenderbuffers(1, &RBO);
	glDeleteTextures(2, textures);
	glDeleteFramebuffers(2, FBOs);

	return strdup(filename);
}

void game_on_resize(int width, int height)
{
	window_width = width;
	window_height = height;
}
