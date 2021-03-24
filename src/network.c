#include <poll.h>

bool send_command(Client *client, RemoteCommand cmd)
{
	pthread_mutex_lock(client->write_mtx);
	bool ret = write_u32(client->fd, cmd);
	pthread_mutex_unlock(client->write_mtx);
	return ret;
}

static void handle_packets(Client *client) {
	while (client->state != CS_DISCONNECTED) {
		struct pollfd pfd = {
			.fd = client->fd,
			.events = POLLIN,
			.revents = 0,
		};

		int pstate = poll(&pfd, 1, 0);

		if (pstate == -1) {
			perror("poll");
			return;
		}

		if (client->state == CS_DISCONNECTED)
			return;

		if (pstate == 0)
			continue;

		if (! (pfd.revents & POLLIN))
			return;

		HostCommand command;
		if (! read_u32(client->fd, &command))
			return;

		CommandHandler *handler = NULL;

		if (command < HOST_COMMAND_COUNT)
			handler = &command_handlers[command];

		if (handler && handler->func) {
			bool good = client->state & handler->state_flags;
			if (! good)
				printf("Recieved %s command, but client is in invalid state: %d\n", handler->name, client->state);
			if (! handler->func(client, good))
				return;
		} else {
			printf("Recieved invalid command %d\n", command);
		}
	}
}
