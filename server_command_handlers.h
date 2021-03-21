#ifndef _SERVER_COMMAND_HANDLERS_H_
#define _SERVER_COMMAND_HANDLERS_H_

#include "server.h"

typedef struct {
	bool (*func)(Client *client);
	const char *name;
	int state_flags;
} ServerCommandHandler;

extern ServerCommandHandler server_command_handlers[];

#endif
