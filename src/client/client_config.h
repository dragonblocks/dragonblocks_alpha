#ifndef _CLIENT_CONFIG_H_
#define _CLIENT_CONFIG_H_

#include <stdbool.h>

extern struct ClientConfig {
	unsigned int antialiasing;
	bool mipmap;
	double view_distance;
	bool vsync;
	unsigned int meshgen_threads;
	bool swap_mouse_buttons;
	bool texture_batching;
} client_config;

#endif
