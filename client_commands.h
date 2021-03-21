#ifndef _CLIENT_COMMAND_H_
#define _CLIENT_COMMAND_H_

typedef enum
{
	CC_DISCONNECT,
	CC_AUTH_SUCCESS,
	CC_AUTH_FAILURE,
	CC_BLOCK,
} ClientCommand;

#endif
