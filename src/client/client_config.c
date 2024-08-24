#include "client/client_config.h"
#include "common/config.h"

struct ClientConfig client_config = {
	.antialiasing = 4,
	.mipmap = true,
	.view_distance = 255.0,
	.vsync = true,
	.meshgen_threads = 4,
	.swap_mouse_buttons = false,
	.swap_gamepad_buttons = true,
	.gamepad_deadzone = 0.1,
	.gamepad_sensitivity = 12.5,
	.atlas_size = 1024,
	.atlas_mipmap = 4,
};

#define CONFIG_ENTRY(T, X) { CONFIG_##T, #X, &client_config.X }

static ConfigEntry config_entries[] = {
	CONFIG_ENTRY(UINT, antialiasing),
	CONFIG_ENTRY(BOOL, mipmap),
	CONFIG_ENTRY(FLOAT, view_distance),
	CONFIG_ENTRY(BOOL, vsync),
	CONFIG_ENTRY(UINT, meshgen_threads),
	CONFIG_ENTRY(BOOL, swap_mouse_buttons),
	CONFIG_ENTRY(BOOL, swap_gamepad_buttons),
	CONFIG_ENTRY(FLOAT, gamepad_deadzone),
	CONFIG_ENTRY(FLOAT, gamepad_sensitivity),
	CONFIG_ENTRY(UINT, atlas_size),
	CONFIG_ENTRY(UINT, atlas_mipmap),
};

void client_config_load(const char *path)
{
	config_read(path, config_entries, sizeof config_entries / sizeof *config_entries);
}

__attribute__((destructor)) static void client_config_deinit()
{
	config_free(config_entries, sizeof config_entries / sizeof *config_entries);
}
