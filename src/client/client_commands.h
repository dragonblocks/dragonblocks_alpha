#ifndef _CLIENT_COMMANDS_H_
#define _CLIENT_COMMANDS_H_

typedef enum
{
	CLIENT_COMMAND_NULL,
	CC_DISCONNECT,
	CC_AUTH,
	CC_BLOCK,
	CC_SIMULATION_DISTANCE,
	CLIENT_COMMAND_COUNT
} ClientCommand;

#ifdef _SERVER_H_
typedef ClientCommand RemoteCommand;
#endif

#ifdef _CLIENT_H_
typedef ClientCommand HostCommand;
#define HOST_COMMAND_COUNT CLIENT_COMMAND_COUNT
#endif

#endif
