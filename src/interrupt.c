#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "interrupt.h"

Flag interrupt;
static struct sigaction sa = {0};

static void interrupt_handler(int sig)
{
	fprintf(stderr, "%s\n", strsignal(sig));
	flag_set(&interrupt);
}

void interrupt_init()
{
	flag_ini(&interrupt);

	sa.sa_handler = &interrupt_handler;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
}

void interrupt_deinit()
{
	flag_dst(&interrupt);
}
