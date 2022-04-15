#ifndef _GAME_H_
#define _GAME_H_

#include <dragonstd/flag.h>

extern int game_fps;

bool game(Flag *gfx_init);
char *game_take_screenshot();

#endif // _GAME_H_
