#ifndef _INTERACT_H_
#define _INTERACT_H_

extern struct InteractPointed {
	bool exists;
	v3s32 pos;
	NodeType node;
} interact_pointed;

bool interact_init();
void interact_deinit();
void interact_tick();
void interact_render();

#endif // _INTERACT_H_
