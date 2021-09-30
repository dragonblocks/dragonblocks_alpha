#ifndef _INPUT_H_
#define _INPUT_H_

#include <dragontype/number.h>

void input_tick(f64 dtime);
void input_init();
void input_on_cursor_pos(double current_x, double current_y);

#endif
