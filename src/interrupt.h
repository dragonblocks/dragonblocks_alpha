#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#include <dragonstd/flag.h>

extern Flag *interrupt;

void interrupt_init();
void interrupt_deinit();

#endif
