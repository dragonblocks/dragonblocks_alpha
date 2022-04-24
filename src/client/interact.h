#ifndef _INTERACT_H_
#define _INTERACT_H_

#include <stdbool.h>
#include "node.h"
#include "types.h"

extern struct InteractPointed {
	bool exists;
	v3s32 pos;
	NodeType node;
} interact_pointed;

void interact_init();
void interact_deinit();
void interact_tick();
void interact_render();
void interact_use(bool left);

#endif // _INTERACT_H_
