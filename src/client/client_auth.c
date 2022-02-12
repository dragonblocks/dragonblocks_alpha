#include <stddef.h>
#include <stdio.h>
#include <linenoise/linenoise.h>
#include "client.h"
#include "client_auth.h"
#include "signal_handlers.h"
#include "types.h"

struct ClientAuth client_auth;

static bool name_prompt()
{
	if (! (client_auth.name = linenoise("Enter name: ")))
		return false;

	dragonnet_peer_send_ToServerAuth(client, &(ToServerAuth) {
		.name = client_auth.name,
	});

	printf("Authenticating as %s...\n", client_auth.name);
	client_auth.state = AUTH_WAIT;

	return true;
}

bool client_auth_init()
{
	client_auth.state = AUTH_INIT;

	while (! interrupted) {
		switch (client_auth.state) {
			case AUTH_INIT:
				if (name_prompt())
					break;
				else
					return false;

			case AUTH_WAIT:
				sched_yield();
				break;

			case AUTH_SUCCESS:
				return true;
		}
	}

	return false;
}

void client_auth_deinit()
{
	linenoiseFree(client_auth.name);
}
