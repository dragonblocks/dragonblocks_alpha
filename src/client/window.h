#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <GLFW/glfw3.h>

extern struct Window
{
	GLFWwindow *handle;
	bool fullscreen;
	struct
	{
		int x, y;
		int width, height;
	} small_bounds;
} window;

bool window_init(int width, int height);
void window_enter_fullscreen();
void window_exit_fullscreen();

#endif
