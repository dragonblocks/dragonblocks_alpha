#include <stdio.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include "client/debug_menu.h"
#include "client/hud.h"
#include "client/input.h"
#include "client/scene.h"
#include "client/window.h"
#include "util.h"

struct Window window;

static void framebuffer_size_callback(unused GLFWwindow *handle, int width, int height)
{
	glViewport(0, 0, width, height);

	if (! window.fullscreen) {
		window.small_bounds.width = width;
		window.small_bounds.height = height;
	}

	scene_on_resize(width, height);
	hud_on_resize(width, height);
}

static void cursor_pos_callback(unused GLFWwindow *handle, double current_x, double current_y)
{
	input_on_cursor_pos(current_x, current_y);
}

static void window_pos_callback(unused GLFWwindow *handle, int x, int y)
{
	if (! window.fullscreen) {
		window.small_bounds.x = x;
		window.small_bounds.y = y;
	}
}

void window_enter_fullscreen()
{
	window.fullscreen = true;
	GLFWmonitor *monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode *vidmode = glfwGetVideoMode(monitor);
	glfwSetWindowMonitor(window.handle, monitor, 0, 0, vidmode->width, vidmode->height, 0);

	debug_menu_update_fullscreen();
}

void window_exit_fullscreen()
{
	window.fullscreen = false;
	glfwSetWindowMonitor(window.handle, NULL, window.small_bounds.x, window.small_bounds.y, window.small_bounds.width, window.small_bounds.height, 0);

	debug_menu_update_fullscreen();
}

bool window_init(int width, int height)
{
	if(! glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW\n");
		return false;
	}

	glfwWindowHint(GLFW_SAMPLES, 8);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window.handle = glfwCreateWindow(width, height, "Dragonblocks", NULL, NULL);

	window.small_bounds.width = width;
	window.small_bounds.height = height;

	if (! window.handle) {
		fprintf(stderr, "Failed to create window\n");
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(window.handle);

	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return false;
	}

	glfwSetFramebufferSizeCallback(window.handle, &framebuffer_size_callback);
	glfwSetCursorPosCallback(window.handle, &cursor_pos_callback);
	glfwSetWindowPosCallback(window.handle, &window_pos_callback);

	return true;
}
