#ifndef _DEBUG_MENU_H_
#define _DEBUG_MENU_H_

#include <stdbool.h>

void debug_menu_init();
void debug_menu_toggle();

void debug_menu_update_version();
void debug_menu_update_fps(int fps);
void debug_menu_update_pos();
void debug_menu_update_yaw();
void debug_menu_update_pitch();
void debug_menu_update_humidity();
void debug_menu_update_temperature();
void debug_menu_update_flight();
void debug_menu_update_collision();
void debug_menu_update_fullscreen();
void debug_menu_update_opengl();
void debug_menu_update_gpu();
void debug_menu_update_glsl();

#endif
