#ifndef _INPUT_H_
#define _INPUT_H_

#include <stdbool.h>
#include "types.h"

void input_init();
void input_tick(f64 dtime);
void input_cursor(double current_x, double current_y);
void input_click(bool left);

#endif // _INPUT_H_
