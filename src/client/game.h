#ifndef _GAME_H_
#define _GAME_H_

#include "client/client.h"

bool game(Client *client);
char *take_screenshot();
void game_on_resize(int width, int height);

#endif
