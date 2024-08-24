#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#include <dragonstd/flag.h>

void interrupt_init();
void interrupt_deinit();
void interrupt_exit_on_eof();

extern Flag interrupt;

#endif // _INTERRUPT_H_
