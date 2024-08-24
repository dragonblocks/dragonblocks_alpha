#ifndef _ACTION_H_
#define _ACTION_H_

#include <stdbool.h>
#include "types.h"

void action_init();
void action_tick(f64 dtime);
void action_menu_navigate(int dir);
void action_menu_set_open(bool open);
void action_menu_select();

// actions
void action_screenshot();
void action_toggle_fullscreen();
void action_toggle_flight();
void action_toggle_debug_menu();
void action_toggle_timelapse();
void action_toggle_collision();
void action_quit();

#endif
