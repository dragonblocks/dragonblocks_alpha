#ifndef _SERVER_CONFIG_H_
#define _SERVER_CONFIG_H_

extern struct ServerConfig {
	unsigned int load_distance;
	unsigned int terrain_gen_threads;
	struct {
		double speed_normal;
		double speed_flight;
		double gravity;
		double jump;

		// allow_op, allow_all, force_on, force_off
	} movement;
} server_config;

void server_config_load(const char *path);

#endif // _SERVER_CONFIG_H_
