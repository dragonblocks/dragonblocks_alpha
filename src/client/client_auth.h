#ifndef _CLIENT_AUTH_H_
#define _CLIENT_AUTH_H_

typedef enum
{
	AUTH_INIT,
	AUTH_WAIT,
	AUTH_SUCCESS,
} ClientAuthState;

extern struct ClientAuth
{
	char *name;
	ClientAuthState state;
} client_auth;

bool client_auth_init();
void client_auth_assert_state(ClientAuthState state, const char *pkt);
void client_auth_deinit();

#endif
