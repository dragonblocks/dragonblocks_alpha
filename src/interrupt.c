#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "interrupt.h"

Flag interrupt;

#ifdef _WIN32

// just the signals we need, for Win32
static const char *strsignal(int sig)
{
	switch (sig) {
		case SIGINT:
			return "Interrupted";

		case SIGTERM:
			return "Terminated";

		default:
			return "Unknown signal";
	}
}

#endif // _WIN32

static void interrupt_handler(int sig)
{
	fprintf(stderr, "%s\n", strsignal(sig));
	flag_set(&interrupt);
}

void interrupt_init()
{
	flag_ini(&interrupt);

#ifdef _WIN32
	signal(SIGINT, interrupt_handler);
	signal(SIGTERM, interrupt_handler);
#else // _WIN32
	struct sigaction sa = {0};
	sa.sa_handler = &interrupt_handler;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
#endif // _WIN32
}

void interrupt_deinit()
{
	flag_dst(&interrupt);
}
