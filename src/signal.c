#include <signal.h>
#include <stdio.h>
#include <string.h>

static void interrupt_handler(int sig)
{
	fprintf(stderr, "%s\n", strsignal(sig));
}

static void silent_handler(int sig)
{
	(void) sig;
}

static struct sigaction sigact_interrupt = {0};
static struct sigaction sigact_silent = {0};

void init_signal_handlers()
{
	sigact_interrupt.sa_handler = &interrupt_handler;
	sigaction(SIGINT, &sigact_interrupt, NULL);
	sigaction(SIGTERM, &sigact_interrupt, NULL);

	sigact_silent.sa_handler = &silent_handler;
	sigaction(SIGPIPE, &sigact_silent, NULL);
}
