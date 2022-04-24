#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <GLFW/glfw3.h>
#include <linmath.h/linmath.h>
#include <stdbool.h>
#include "types.h"

extern struct Window {
	int width, height;
	GLFWwindow *handle;
	bool fullscreen;
	f32 fov;
	mat4x4 projection;
} window;

void window_init();
void window_enter_fullscreen();
void window_exit_fullscreen();

#endif // _WINDOW_H_
