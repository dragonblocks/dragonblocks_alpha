#ifndef _SERVER_COMMAND_H_
#define _SERVER_COMMAND_H_

typedef enum
{
	SERVER_COMMAND_NULL,
	SC_DISCONNECT,
	SC_AUTH,
	SC_GETBLOCK,
	SC_SETNODE,
	SERVER_COMMAND_COUNT,
} ServerCommand;

#ifdef _CLIENT_H_
typedef ServerCommand RemoteCommand;
#endif

#ifdef _SERVER_H_
typedef ServerCommand HostCommand;
#define HOST_COMMAND_COUNT SERVER_COMMAND_COUNT
#endif

#endif
