#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include "client/client_config.h"
#include "client/debug_menu.h"
#include "client/game.h"
#include "client/opengl.h"
#include "client/gui.h"
#include "client/input.h"
#include "client/window.h"

struct Window window;

static int small_x, small_y, small_width, small_height;

static void update_projection()
{
	mat4x4_perspective(window.projection,
		window.fov / 180.0f * M_PI,
		(float) window.width / (float) window.height,
		0.01f, client_config.view_distance + 28.0f);
}

static void framebuffer_size_callback(__attribute__((unused)) GLFWwindow *handle, int width, int height)
{
	glViewport(0, 0, width, height); GL_DEBUG
	window.width = width;
	window.height = height;

	if (!window.fullscreen) {
		small_width = width;
		small_height = height;
	}

	update_projection();
	gui_update_projection();
}

static void cursor_pos_callback(__attribute__((unused)) GLFWwindow *handle, double x, double y)
{
	input_cursor(x, y);
}

static void window_pos_callback(__attribute__((unused)) GLFWwindow *handle, int x, int y)
{
	if (!window.fullscreen) {
		small_x = x;
		small_y = y;
	}
}

static void mouse_button_callback(__attribute__((unused)) GLFWwindow *handle, int button, int action, __attribute__((unused)) int mods)
{
	if ((button == GLFW_MOUSE_BUTTON_RIGHT || button == GLFW_MOUSE_BUTTON_LEFT) && action == GLFW_PRESS)
		input_click(button == GLFW_MOUSE_BUTTON_RIGHT);
}

static void error_callback(__attribute__((unused)) int error, const char *description)
{
	fprintf(stderr, "[warning] GLFW error: %s\n", description);
}

void window_enter_fullscreen()
{
	window.fullscreen = true;
	GLFWmonitor *monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode *vidmode = glfwGetVideoMode(monitor);
	glfwSetWindowMonitor(window.handle, monitor, 0, 0, vidmode->width, vidmode->height, vidmode->refreshRate);

	debug_menu_changed(ENTRY_FULLSCREEN);
}

void window_exit_fullscreen()
{
	window.fullscreen = false;
	glfwSetWindowMonitor(window.handle, NULL, small_x, small_y, small_width, small_height, 0);

	debug_menu_changed(ENTRY_FULLSCREEN);
}

void window_init()
{
	if(!glfwInit()) {
		fprintf(stderr, "[error] failed to initialize GLFW\n");
		abort();
	}

	glfwSetErrorCallback(&error_callback);

	glfwWindowHint(GLFW_SAMPLES, client_config.antialiasing);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef GLFW_WAYLAND_APP_ID
	glfwWindowHintString(GLFW_WAYLAND_APP_ID, "dragonblocks_alpha");
#endif
	glfwWindowHintString(GLFW_X11_CLASS_NAME, "dragonblocks_alpha");
	glfwWindowHintString(GLFW_X11_INSTANCE_NAME, "dragonblocks_alpha");

	window.width = 1250;
	window.height = 750;
	window.handle = glfwCreateWindow(window.width, window.height, "Dragonblocks", NULL, NULL);
	window.fullscreen = false;
	window.fov = 86.1f;
	update_projection();

	small_width = window.width;
	small_height = window.height;

	if (!window.handle) {
		fprintf(stderr, "[error] failed to create window (does your machine support OpenGL 3.3?)\n");
		glfwTerminate();
		abort();
	}

	glfwMakeContextCurrent(window.handle);

	if (!client_config.vsync)
		glfwSwapInterval(0);

	GLenum err = glewInit();
	if (err != GLEW_OK) {
		fprintf(stderr, "[error] failed to initialize GLEW: %s\n", glewGetErrorString(err));
		abort();
	}

	glfwSetFramebufferSizeCallback(window.handle, &framebuffer_size_callback);
	glfwSetCursorPosCallback(window.handle, &cursor_pos_callback);
	glfwSetWindowPosCallback(window.handle, &window_pos_callback);
	glfwSetMouseButtonCallback(window.handle, &mouse_button_callback);
}
