#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <GLFW/glfw3.h>

extern struct Window {
	int width, height;
	GLFWwindow *handle;
	bool fullscreen;
	f32 fov;
	mat4x4 projection;
} window;

bool window_init();
void window_enter_fullscreen();
void window_exit_fullscreen();

#endif // _WINDOW_H_
