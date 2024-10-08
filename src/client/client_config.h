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
	bool swap_gamepad_buttons;
	double gamepad_deadzone;
	double gamepad_sensitivity;
	unsigned int atlas_size;
	unsigned int atlas_mipmap;
} client_config;

void client_config_load(const char *path);

#endif
