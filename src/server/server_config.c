#include "common/config.h"
#include "server/server_config.h"

struct ServerConfig server_config = {
	.load_distance = 10,
	.terrain_gen_threads = 4,
	.movement = {
		.speed_normal = 4.317,
		.speed_flight = 25.0,
		.gravity = 32.0,
		.jump = 8.944,
	}
};

static ConfigEntry config_entries[] = {
	{
		.type = CONFIG_UINT,
		.key = "load_distance",
		.value = &server_config.load_distance,
	},
	{
		.type = CONFIG_UINT,
		.key = "terrain_gen_threads",
		.value = &server_config.terrain_gen_threads,
	},
	{
		.type = CONFIG_FLOAT,
		.key = "movement.speed_normal",
		.value = &server_config.movement.speed_normal,
	},
	{
		.type = CONFIG_FLOAT,
		.key = "movement.speed_flight",
		.value = &server_config.movement.speed_flight,
	},
	{
		.type = CONFIG_FLOAT,
		.key = "movement.gravity",
		.value = &server_config.movement.gravity,
	},
	{
		.type = CONFIG_FLOAT,
		.key = "movement.jump",
		.value = &server_config.movement.jump,
	},
};

__attribute__((constructor)) static void server_config_init()
{
	config_read("server.conf", config_entries, sizeof config_entries / sizeof *config_entries);
}

__attribute__((destructor)) static void server_config_deinit()
{
	config_free(config_entries, sizeof config_entries / sizeof *config_entries);
}
