#include "client/client_config.h"
#include "common/config.h"

struct ClientConfig client_config = {
	.antialiasing = 4,
	.mipmap = true,
	.view_distance = 255.0,
	.vsync = true,
	.meshgen_threads = 4,
	.swap_mouse_buttons = true,
	.atlas_size = 1024,
	.atlas_mipmap = 4,
};

static ConfigEntry config_entries[] = {
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
	{
		.type = CONFIG_BOOL,
		.key = "swap_mouse_buttons",
		.value = &client_config.meshgen_threads,
	},
	{
		.type = CONFIG_UINT,
		.key = "atlas_size",
		.value = &client_config.atlas_size,
	},
	{
		.type = CONFIG_UINT,
		.key = "atlas_mipmap",
		.value = &client_config.atlas_mipmap,
	},
};

__attribute__((constructor)) static void client_config_init()
{
	config_read("client.conf", config_entries, sizeof config_entries / sizeof *config_entries);
}

__attribute__((destructor)) static void client_config_deinit()
{
	config_free(config_entries, sizeof config_entries / sizeof *config_entries);
}
