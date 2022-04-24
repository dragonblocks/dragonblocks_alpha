#ifndef _GAME_H_
#define _GAME_H_

#include <dragonstd/flag.h>
#include "types.h"

extern int game_fps;

void game(Flag *gfx_init);
void game_render(f64 dtime);

#endif // _GAME_H_
