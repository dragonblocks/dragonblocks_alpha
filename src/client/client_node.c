#include "client/client.h"
#include "client/client_node.h"
#include "color.h"
#include "environment.h"
#include "node.h"
#include "perlin.h"

#define TILES_SIMPLE(path) {.paths = {path, NULL, NULL, NULL, NULL, NULL}, .indices = {0, 0, 0, 0, 0, 0}, .textures = {NULL}}
#define TILES_NONE {.paths = {NULL}, .indices = {0}, .textures = {NULL}}

static void render_grass(NodeArgsRender *args)
{
	args->vertex.color = hsl_to_rgb((v3f32) {f32_mix(
		// hue values between .13 and .33 depending on humidity
		f32_mix(
			0.13f,
			0.33f,
			get_humidity(args->pos)
		),
		// move towards .45 while temperature is between .3 and .0
		0.45f,
		f32_clamp(
			0.3f - get_temperature(args->pos),
			0.0f,
			0.3f
		) / 0.3f
	), 1.0f, 0.5f});
}

static void render_stone(NodeArgsRender *args)
{
	args->vertex.cube.textureCoordinates.x += noise2d(args->pos.x, args->pos.z, 0, seed + OFFSET_TEXTURE_OFFSET_S);
	args->vertex.cube.textureCoordinates.y += noise2d(args->pos.x, args->pos.z, 0, seed + OFFSET_TEXTURE_OFFSET_T);
}

static void render_color(NodeArgsRender *args)
{
	args->vertex.color = ((ColorData *) args->node->data)->color;
}

ClientNodeDefinition client_node_definitions[NODE_UNLOADED] = {
	// unknown
	{
		.tiles = TILES_SIMPLE(RESSOURCE_PATH "textures/unknown.png"),
		.visibility = VISIBILITY_SOLID,
		.mipmap = true,
		.render = NULL,
	},
	// air
	{
		.tiles = TILES_NONE,
		.visibility = VISIBILITY_NONE,
		.mipmap = true,
		.render = NULL,
	},
	// grass
	{
		.tiles = TILES_SIMPLE(RESSOURCE_PATH "textures/grass.png"),
		.visibility = VISIBILITY_SOLID,
		.mipmap = true,
		.render = &render_grass,
	},
	// dirt
	{
		.tiles = TILES_SIMPLE(RESSOURCE_PATH "textures/dirt.png"),
		.visibility = VISIBILITY_SOLID,
		.mipmap = true,
		.render = NULL,
	},
	// stone
	{
		.tiles = TILES_SIMPLE(RESSOURCE_PATH "textures/stone.png"),
		.visibility = VISIBILITY_SOLID,
		.mipmap = true,
		.render = &render_stone,
	},
	// snow
	{
		.tiles = TILES_SIMPLE(RESSOURCE_PATH "textures/snow.png"),
		.visibility = VISIBILITY_SOLID,
		.mipmap = true,
		.render = NULL,
	},
	// oak wood
	{
		.tiles = {
			.paths = {RESSOURCE_PATH "textures/oak_wood.png", RESSOURCE_PATH "textures/oak_wood_top.png", NULL, NULL, NULL, NULL},
			.indices = {0, 0, 0, 0, 1, 1},
			.textures = {NULL},
		},
		.visibility = VISIBILITY_SOLID,
		.mipmap = true,
		.render = &render_color,
	},
	// oak leaves
	{
		.tiles = TILES_SIMPLE(RESSOURCE_PATH "textures/oak_leaves.png"),
		.visibility = VISIBILITY_SOLID,
		.mipmap = true,
		.render = &render_color,
	},
	// pine wood
	{
		.tiles = {
			.paths = {RESSOURCE_PATH "textures/pine_wood.png", RESSOURCE_PATH "textures/pine_wood_top.png", NULL, NULL, NULL, NULL},
			.indices = {0, 0, 0, 0, 1, 1},
			.textures = {NULL},
		},
		.visibility = VISIBILITY_SOLID,
		.mipmap = true,
		.render = &render_color,
	},
	// pine leaves
	{
		.tiles = TILES_SIMPLE(RESSOURCE_PATH "textures/pine_leaves.png"),
		.visibility = VISIBILITY_CLIP,
		.mipmap = true,
		.render = &render_color,
	},
	// palm wood
	{
		.tiles = {
			.paths = {RESSOURCE_PATH "textures/palm_wood.png", RESSOURCE_PATH "textures/palm_wood_top.png", NULL, NULL, NULL, NULL},
			.indices = {0, 0, 0, 0, 1, 1},
			.textures = {NULL},
		},
		.visibility = VISIBILITY_SOLID,
		.mipmap = true,
		.render = &render_color,
	},
	// palm leaves
	{
		.tiles = TILES_SIMPLE(RESSOURCE_PATH "textures/palm_leaves.png"),
		.visibility = VISIBILITY_SOLID,
		.mipmap = true,
		.render = &render_color,
	},
	// sand
	{
		.tiles = TILES_SIMPLE(RESSOURCE_PATH "textures/sand.png"),
		.visibility = VISIBILITY_SOLID,
		.mipmap = true,
		.render = NULL,
	},
	// water
	{
		.tiles = TILES_SIMPLE(RESSOURCE_PATH "textures/water.png"),
		.visibility = VISIBILITY_BLEND,
		.mipmap = true,
		.render = NULL,
	},
	// lava
	{
		.tiles = TILES_SIMPLE(RESSOURCE_PATH "textures/lava.png"),
		.visibility = VISIBILITY_BLEND,
		.mipmap = true,
		.render = NULL,
	},
	// vulcano_stone
	{
		.tiles = TILES_SIMPLE(RESSOURCE_PATH "textures/vulcano_stone.png"),
		.visibility = VISIBILITY_SOLID,
		.mipmap = true,
		.render = NULL,
	},
};

void client_node_init()
{
	for (NodeType node = NODE_UNKNOWN; node < NODE_UNLOADED; node++) {
		ClientNodeDefinition *def = &client_node_definitions[node];

		if (def->visibility != VISIBILITY_NONE) {
			Texture *textures[6];

			for (int i = 0; i < 6; i++) {
				char *path = def->tiles.paths[i];

				if (path)
					textures[i] = texture_load(path, def->mipmap);
				else
					break;
			}

			for (int i = 0; i < 6; i++)
				def->tiles.textures[i] = textures[def->tiles.indices[i]];
		}
	}
}
