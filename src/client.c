#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include "client.h"
#include "mesh.h"
#include "signal.h"
#include "util.h"

void client_create_mesh(Client *client, Mesh *mesh);
{
	pthread_mutex_lock(client->meshlist_mtx);
	list_put(&client->meshlist, mesh, NULL);
	pthread_mutex_unlock(client->meshlist_mtx);
}

void client_remove_mesh(Client *client, Mesh *mesh);
{
	pthread_mutex_lock(client->meshlist_mtx);
	list_delete(&client->meshlist, mesh);
	pthread_mutex_unlock(client->meshlist_mtx);
	delete_mesh(mesh);
}

void client_mapblock_changed(Client *client, v3s32 pos)
{
	v3s32 *posptr = malloc(sizeof(v3s32));
	*posptr = pos;
	pthread_mutex_lock(client->mapblock_meshgen_mtx);
	if (! list_put(&client->mapblock_meshgen_queue, posptr, NULL))
		free(posptr);
	pthread_mutex_unlock(client->mapblock_meshgen_mtx);
}

void client_disconnect(Client *client, bool send, const char *detail)
{
	pthread_mutex_lock(client->write_mtx);
	if (client->state != CS_DISCONNECTED) {
		if (send)
			write_u32(client->fd, SC_DISCONNECT);

		client->state = CS_DISCONNECTED;
		printf("Disconnected %s%s%s\n", INBRACES(detail));
		close(client->fd);
	}
	pthread_mutex_unlock(client->write_mtx);
}

#include "network.c"

static void *reciever_thread(void *cliptr)
{
	Client *client = cliptr;

	handle_packets(client);

	if (errno == EINTR)
		client_disconnect(client, true, NULL);
	else
		client_disconnect(client, false, "network error");

	if (client->name)
		free(client->name);

	pthread_mutex_lock(client->meshlist_mtx);
	ITERATE_LIST(&client->meshes, pair) delete_mesh(pair->value);
	list_clear(&client->meshes);
	pthread_mutex_unlock(client->meshlist_mtx);

	for (int i = 0; i < CLIENT_MTX_COUNT; i++)
		pthread_mutex_destroy(&client.mutexes[i]);

	exit(EXIT_SUCCESS);
	return NULL;
}

static void *mapblock_meshgen_thread(void *cliptr)
{
	Client *client = cliptr;

	for ever {
		ListPair **lptr = &client->mapblock_meshgen_queue.first;
		if (*lptr) {
			MapBlock *block = map_get_block(client->map, *(v3s32 *)(*lptr)->key, false);
			Array vertices(sizeof(GLfloat));

			// ToDo: Actual vertices generation code

			client_create_mesh(client, create_mesh(vertices.ptr, vertices.siz));
			pthread_mutex_lock(client->mapblock_meshgen_mtx);
			*lptr = (*lptr)->next;
			pthread_mutex_unlock(client->mapblock_meshgen_mtx);
		} else {
			sched_yield();
		}
	}
}

static void client_loop(Client *client)
{
	if(! glfwInit()) {
		printf("Failed to initialize GLFW\n");
		return;
	}

	glfwWindowHint(GLFW_SAMPLES, 8);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow *window = glfwCreateWindow(1024, 768, "Dragonblocks", NULL, NULL);

	if (! window) {
		printf("Failed to create window\n");
		glfwTerminate();
		return;
	}

	glfwMakeContextCurrent(window);
	if (glewInit() != GLEW_OK) {
		printf("Failed to initialize GLEW\n");
		return;
	}

	while (! glfwWindowShouldClose(window) && client->state != CS_DISCONNECTED && ! interrupted) {
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.52941176470588, 0.8078431372549, 0.92156862745098, 1.0);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

static bool client_name_prompt(Client *client)
{
	printf("Enter name: ");
	fflush(stdout);
	char name[NAME_MAX];
	if (scanf("%s", name) == EOF)
		return false;
	client->name = strdup(name);
	pthread_mutex_lock(client->write_mtx);
	if (write_u32(client->fd, SC_AUTH) && write(client->fd, client->name, strlen(name) + 1)) {
		client->state = CS_AUTH;
		printf("Authenticating...\n");
	}
	pthread_mutex_unlock(client->write_mtx);
	return true;
}

static bool client_authenticate(Client *client)
{
	for ever {
		switch (client->state) {
			case CS_CREATED:
				if (client_name_prompt(client))
					break;
				else
					return false;
			case CS_AUTH:
				if (interrupted)
					return false;
				else
					sched_yield();
				break;
			case CS_ACTIVE:
				return true;
			case CS_DISCONNECTED:
				return false;
		}
	}
	return false;
}

static bool compare_positions(void *p1, void *p2)
{
	v3s32 *pos1 = p1;
	v3s32 *pos2 = p2;
	return pos1->x == pos2->x && pos1->y == pos2->y && pos->z == pos2->z;
}

int main(int argc, char **argv)
{
	program_name = argv[0];

	if (argc < 3)
		internal_error("missing address or port");

	struct addrinfo hints = {
		.ai_family = AF_UNSPEC,			// support both IPv4 and IPv6
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = 0,
		.ai_flags = AI_NUMERICSERV,
	};

	struct addrinfo *info = NULL;

	int gai_state = getaddrinfo(argv[1], argv[2], &hints, &info);

	if (gai_state != 0)
		internal_error(gai_strerror(gai_state));

	Client client = {
		.fd = -1,
		.map = NULL,
		.name = NULL,
		.state = CS_CREATED,
		.meshlist = list_create(NULL),
		.mapblock_meshgen_queue = list_create(&compare_positions),
	};

	for (int i = 0; i < CLIENT_MTX_COUNT; i++)
		pthread_mutex_init(&client.mutexes[i], NULL);

	client.write_mtx = &client.mutexes[0];
	client.meshlist_mtx = &client.mutexes[1];
	client.mapblock_meshgen_mtx = &client.mutexes[2];

	client.fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

	if (client.fd == -1)
		syscall_error("socket");

	if (connect(client.fd, info->ai_addr, info->ai_addrlen) == -1)
		syscall_error("connect");

	char *addrstr = address_string((struct sockaddr_in6 *) info->ai_addr);
	printf("Connected to %s\n", addrstr);
	free(addrstr);

	freeaddrinfo(info);

	init_signal_handlers();

	client.map = map_create();

	pthread_t recv_thread;
	pthread_t mmg_thread;
	pthread_create(&recv_thread, NULL, &reciever_thread, &client);
	pthread_create(&mmg_thread, NULL, &mapblock_meshgen_thread, &client);

	if (client_authenticate(&client))
		client_loop(&client);

	client_disconnect(&client, true, NULL);

	pthread_join(recv_thread, NULL);
}
