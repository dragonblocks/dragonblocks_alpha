#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "signal_handlers.h"

bool interrupted = false;

static void interrupt_handler(int sig)
{
	interrupted = true;
	fprintf(stderr, "%s\n", strsignal(sig));
}

static void silent_handler(__attribute__((unused)) int sig)
{
}

static struct sigaction sigact_interrupt = {0};
static struct sigaction sigact_silent = {0};

void signal_handlers_init()
{
	sigact_interrupt.sa_handler = &interrupt_handler;
	sigaction(SIGINT, &sigact_interrupt, NULL);
	sigaction(SIGTERM, &sigact_interrupt, NULL);

	sigact_silent.sa_handler = &silent_handler;
	sigaction(SIGPIPE, &sigact_silent, NULL);
}
