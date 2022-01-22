#include "config.h"
#include "client/client_config.h"

struct ClientConfig client_config = {
	.antialiasing = 4,
	.mipmap = true,
	.render_distance = 255.0,
};

__attribute__((constructor)) static void client_config_init()
{
	config_read("client.conf", (ConfigEntry[]) {
		{
			.type = CT_UINT,
			.key = "antialiasing",
			.value = &client_config.antialiasing,
		},
		{
			.type = CT_BOOL,
			.key = "mipmap",
			.value = &client_config.mipmap,
		},
		{
			.type = CT_FLOAT,
			.key = "render_distance",
			.value = &client_config.render_distance,
		}
	}, 3);
}

