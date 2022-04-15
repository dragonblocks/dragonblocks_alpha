#include "config.h"
#include "client/client_config.h"

struct ClientConfig client_config = {
	.antialiasing = 4,
	.mipmap = true,
	.view_distance = 255.0,
	.vsync = true,
	.meshgen_threads = 4,
};

#define NUM_CONFIG_ENTRIES 5
static ConfigEntry config_entries[NUM_CONFIG_ENTRIES] = {
	{
		.type = CONFIG_UINT,
		.key = "antialiasing",
		.value = &client_config.antialiasing,
	},
	{
		.type = CONFIG_BOOL,
		.key = "mipmap",
		.value = &client_config.mipmap,
	},
	{
		.type = CONFIG_FLOAT,
		.key = "view_distance",
		.value = &client_config.view_distance,
	},
	{
		.type = CONFIG_BOOL,
		.key = "vsync",
		.value = &client_config.vsync,
	},
	{
		.type = CONFIG_UINT,
		.key = "meshgen_threads",
		.value = &client_config.meshgen_threads,
	},
};

__attribute__((constructor)) static void client_config_init()
{
	config_read("client.conf", config_entries, NUM_CONFIG_ENTRIES);
}

__attribute__((destructor)) static void client_config_deinit()
{
	config_free(config_entries, NUM_CONFIG_ENTRIES);
}
