#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include "client.h"
#include "mapblock_meshgen.h"
#include "signal.h"
#include "shaders.h"
#include "util.h"

Client client;

void client_disconnect(bool send, const char *detail)
{
	pthread_mutex_lock(&client.mtx);
	if (client.state != CS_DISCONNECTED) {
		if (send)
			write_u32(client.fd, SC_DISCONNECT);

		client.state = CS_DISCONNECTED;
		printf("Disconnected %s%s%s\n", INBRACES(detail));
		close(client.fd);
	}
	pthread_mutex_unlock(&client.mtx);
}

#include "network.c"

static void *reciever_thread(void *unused)
{
	(void) unused;

	handle_packets(&client);

	if (errno != EINTR)
		client_disconnect(false, "network error");

	return NULL;
}

static void client_loop()
{
	if(! glfwInit()) {
		printf("Failed to initialize GLFW\n");
		return;
	}

	glfwWindowHint(GLFW_SAMPLES, 8);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	int width, height;
	width = 1024;
	height = 768;

	GLFWwindow *window = glfwCreateWindow(width, height, "Dragonblocks", NULL, NULL);

	if (! window) {
		fprintf(stderr, "Failed to create window\n");
		glfwTerminate();
		return;
	}

	glfwMakeContextCurrent(window);
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return;
	}

	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	const char *shader_path;

#ifdef RELEASE
	shader_path = "shaders";
#else
	shader_path = "../shaders";
#endif

	ShaderProgram *prog = create_shader_program(shader_path);
	if (! prog) {
		fprintf(stderr, "Failed to create shader program\n");
		return;
	}

	mat4x4 view, projection;

	mat4x4_identity(view);	// ToDo: camera
	mat4x4_perspective(projection, 86.1f / 180.0f * M_PI, (float) width / (float) height, 0.01f, 100.0f);

	glUseProgram(prog->id);
	glUniformMatrix4fv(prog->loc_view, 1, GL_FALSE, view[0]);
	glUniformMatrix4fv(prog->loc_projection, 1, GL_FALSE, projection[0]);

	while (! glfwWindowShouldClose(window) && client.state != CS_DISCONNECTED && ! interrupted) {
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.52941176470588f, 0.8078431372549f, 0.92156862745098f, 1.0f);

		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
			pthread_mutex_lock(&client.mtx);
			(void) (write_u32(client.fd, SC_GETBLOCK) && write_v3s32(client.fd, (v3s32) {0, 0, 0}));
			pthread_mutex_unlock(&client.mtx);
		};

		scene_render(client.scene, prog);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	delete_shader_program(prog);
}

static bool client_name_prompt()
{
	printf("Enter name: ");
	fflush(stdout);
	char name[NAME_MAX];
	if (scanf("%s", name) == EOF)
		return false;
	client.name = strdup(name);
	pthread_mutex_lock(&client.mtx);
	if (write_u32(client.fd, SC_AUTH) && write(client.fd, client.name, strlen(client.name) + 1)) {
		client.state = CS_AUTH;
		printf("Authenticating...\n");
	}
	pthread_mutex_unlock(&client.mtx);
	return true;
}

static bool client_authenticate()
{
	for ever {
		switch (client.state) {
			case CS_CREATED:
				if (client_name_prompt())
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

static void client_start(int fd)
{
	client.fd = fd;
	pthread_mutex_init(&client.mtx, NULL);
	client.state = CS_CREATED;
	client.name = NULL;
	client.map = map_create();
	client.scene = scene_create();

	mapblock_meshgen_init(client.map, client.scene);

	pthread_t recv_thread;
	pthread_create(&recv_thread, NULL, &reciever_thread, NULL);

	if (client_authenticate())
		client_loop();

	if (client.state != CS_DISCONNECTED)
		client_disconnect(true, NULL);

	if (client.name)
		free(client.name);

	mapblock_meshgen_stop();

	map_delete(client.map);
	scene_delete(client.scene);

	pthread_mutex_destroy(&client.mtx);
}

int main(int argc, char **argv)
{
	program_name = argv[0];

	if (argc < 3)
		internal_error("missing address or port");

	struct addrinfo hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM,
		.ai_protocol = 0,
		.ai_flags = AI_NUMERICSERV,
	};

	struct addrinfo *info = NULL;

	int gai_state = getaddrinfo(argv[1], argv[2], &hints, &info);

	if (gai_state != 0)
		internal_error(gai_strerror(gai_state));

	int fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

	if (fd == -1)
		syscall_error("socket");

	if (connect(fd, info->ai_addr, info->ai_addrlen) == -1)
		syscall_error("connect");

	char *addrstr = address_string((struct sockaddr_in6 *) info->ai_addr);
	printf("Connected to %s\n", addrstr);
	free(addrstr);

	freeaddrinfo(info);

	init_signal_handlers();
	client_start(fd);

	return EXIT_SUCCESS;
}
