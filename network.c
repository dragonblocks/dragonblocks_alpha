bool send_command(Client *client, RemoteCommand cmd)
{
	pthread_mutex_lock(&client->mtx);
	bool ret = write_u32(client->fd, cmd);
	pthread_mutex_unlock(&client->mtx);
	return ret;
}

static void handle_packets(Client *client) {
	while (client->state != CS_DISCONNECTED) {
		HostCommand command;
		if (! read_u32(client->fd, &command))
			break;

		CommandHandler *handler = NULL;

		if (command < HOST_COMMAND_COUNT)
			handler = &command_handlers[command];

		if (handler && handler->func) {
			bool good = client->state & handler->state_flags;
			if (! good)
				printf("Recieved %s command, but client is in invalid state: %d\n", handler->name, client->state);
			if (! handler->func(client, good))
				break;
		} else {
			printf("Recieved invalid command %d (max = %d)\n", command, HOST_COMMAND_COUNT);
		}
	}
}
