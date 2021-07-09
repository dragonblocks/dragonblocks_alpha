#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <GLFW/glfw3.h>
#include "shaders.h"
#include "types.h"

void init_camera(GLFWwindow *window, ShaderProgram *prog);
void set_camera_position(v3f pos);
void set_window_size(int width, int height);

#endif
