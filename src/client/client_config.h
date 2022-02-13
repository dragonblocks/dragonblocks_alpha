#ifndef _CLIENT_CONFIG_H_
#define _CLIENT_CONFIG_H_

#include <stdbool.h>

extern struct ClientConfig {
	unsigned int antialiasing;
	bool mipmap;
	double render_distance;
	bool vsync;
	unsigned int meshgen_threads;
} client_config;

#endif
