#ifndef _NETWORK_H_
#define _NETWORK_H_

#include <stdbool.h>
#define NAME_MAX 64

typedef enum
{
	CS_CREATED = 0x01,
	CS_AUTH = 0x02,
	CS_ACTIVE = 0x04,
	CS_DISCONNECTED = 0x08,
} ClientState;

struct Client;

typedef struct {
	bool (*func)(struct Client *client, bool good);
	const char *name;
	int state_flags;
} CommandHandler;

extern CommandHandler command_handlers[];

bool send_command(struct Client *client, RemoteCommand cmd);

#endif
