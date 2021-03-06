#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <linenoise.h>
#include "client.h"
#include "client_auth.h"
#include "common/interrupt.h"
#include "types.h"

struct ClientAuth client_auth;

#include <string.h>
static void auth_loop()
{
	while (!interrupt.set) switch (client_auth.state) {
		case AUTH_INIT:
			if (client_auth.name)
				free(client_auth.name);

			if (!(client_auth.name = linenoise("Enter name: ")))
				return;

			printf("[access] authenticating as %s...\n", client_auth.name);
			client_auth.state = AUTH_WAIT;

			dragonnet_peer_send_ToServerAuth(client, &(ToServerAuth) {
				.name = client_auth.name,
			});

			__attribute__((fallthrough));

		case AUTH_WAIT:
			pthread_cond_wait(&client_auth.cv, &client_auth.mtx);
			break;

		case AUTH_SUCCESS:
			return;
	}
}

void client_auth_init()
{
	client_auth.name = NULL;
	pthread_cond_init(&client_auth.cv, NULL);
	pthread_mutex_init(&client_auth.mtx, NULL);

	pthread_mutex_lock(&client_auth.mtx);
	client_auth.state = AUTH_INIT;
	flag_sub(&interrupt, &client_auth.cv); // make sure Ctrl+C will work during AUTH_WAIT

	auth_loop();

	flag_uns(&interrupt, &client_auth.cv);

	if (client_auth.state != AUTH_SUCCESS) {
		fprintf(stderr, "[error] authentication failed due to interruption or read failure\n");
		abort();
	}

	pthread_mutex_unlock(&client_auth.mtx);
}

void client_auth_deinit()
{
	pthread_cond_destroy(&client_auth.cv);
	pthread_mutex_destroy(&client_auth.mtx);
	free(client_auth.name);
}
