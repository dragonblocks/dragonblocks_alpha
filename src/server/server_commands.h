#ifndef _SERVER_COMMANDS_H_
#define _SERVER_COMMANDS_H_

// this file must be included after client.h or server.h and before network.h

typedef enum
{
	SERVER_COMMAND_NULL,	// invalid command
	SC_DISCONNECT,			// client notifies server about disconnecting
	SC_AUTH,				// client wants to authentify [body: name (zero terminated string)]
	SC_SETNODE,				// player placed a node [body: pos (v3s32), node (Node)]
	SC_POS,					// player moved [body: pos (v3f)]
	SC_REQUEST_BLOCK,		// request to send a block [body: pos (v3s32)]
	SERVER_COMMAND_COUNT,	// count of available commands
} ServerCommand;

#ifdef _CLIENT_H_
typedef ServerCommand RemoteCommand;
#endif

#ifdef _SERVER_H_
typedef ServerCommand HostCommand;
#define HOST_COMMAND_COUNT SERVER_COMMAND_COUNT
#endif

#endif
