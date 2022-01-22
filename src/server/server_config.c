#include "config.h"
#include "server/server_config.h"

struct ServerConfig server_config = {
	.simulation_distance = 10,
};

__attribute__((constructor)) static void server_config_init()
{
	config_read("server.conf", (ConfigEntry[]) {
		{
			.type = CT_UINT,
			.key = "simulation_distance",
			.value = &server_config.simulation_distance,
		},
	}, 1);
}

