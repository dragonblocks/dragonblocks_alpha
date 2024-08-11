#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "client.h"
#include "client_auth.h"
#include "common/interrupt.h"
#include "types.h"

struct ClientAuth client_auth;

void client_auth_init()
{
	pthread_cond_init(&client_auth.cv, NULL);
	pthread_mutex_init(&client_auth.mtx, NULL);
	client_auth.state = AUTH_INIT;
}

void client_auth_run(char *name)
{
	pthread_mutex_lock(&client_auth.mtx);

	printf("[access] authenticating as %s...\n", name);

	dragonnet_peer_send_ToServerAuth(client, &(ToServerAuth) {
		.name = name,
	});
	client_auth.state = AUTH_WAIT;

	flag_sub(&interrupt, &client_auth.cv); // make sure Ctrl+C will work during AUTH_WAIT
	pthread_cond_wait(&client_auth.cv, &client_auth.mtx);
	flag_uns(&interrupt, &client_auth.cv);

	if (client_auth.state != AUTH_SUCCESS) {
		fprintf(stderr, "[error] authentication failed\n");
		abort();
	}

	pthread_mutex_unlock(&client_auth.mtx);
}

void client_auth_deinit()
{
	pthread_cond_destroy(&client_auth.cv);
	pthread_mutex_destroy(&client_auth.mtx);
}
