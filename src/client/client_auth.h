#ifndef _CLIENT_AUTH_H_
#define _CLIENT_AUTH_H_

#include <pthread.h>

typedef enum {
	AUTH_INIT,
	AUTH_WAIT,
	AUTH_SUCCESS,
} ClientAuthState;

extern struct ClientAuth {
	char *name;
	ClientAuthState state;
	pthread_cond_t cv;
	pthread_mutex_t mtx;
} client_auth;

void client_auth_init();
void client_auth_deinit();

#endif // _CLIENT_AUTH_H_
