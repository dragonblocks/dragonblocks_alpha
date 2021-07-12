#ifndef _INPUT_H_
#define _INPUT_H_

#include <GLFW/glfw3.h>

void process_input();
void init_input(Client *client, GLFWwindow *window);
void input_on_cursor_pos(double current_x, double current_y);
void input_on_resize(int width, int height);
void input_on_window_pos(int x, int y);

#endif
