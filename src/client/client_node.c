#include <stdlib.h>
#include <math.h>
#include "client/client.h"
#include "client/client_config.h"
#include "client/client_node.h"
#include "common/color.h"
#include "common/environment.h"
#include "common/node.h"
#include "common/perlin.h"

#define TILES_SIMPLE(path) {.paths = {path, NULL, NULL, NULL, NULL, NULL}, .indices = {0, 0, 0, 0, 0, 0}, .textures = {}, .x4 = {}}
#define TILES_NONE {.paths = {NULL}, .indices = {0}, .textures = {}, .x4 = {}}

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
	v2f32 *tcoord = &args->vertex.cube.textureCoordinates;
	tcoord->x = tcoord->x + noise2d(args->pos.x, args->pos.z, 0, seed + OFFSET_TEXTURE_OFFSET_S) * 0.5 + 0.5;
	tcoord->y = tcoord->y + noise2d(args->pos.x, args->pos.z, 0, seed + OFFSET_TEXTURE_OFFSET_T) * 0.5 + 0.5;
	*tcoord = v2f32_scale(*tcoord, 0.5);
}

static void render_color(NodeArgsRender *args)
{
	args->vertex.color = ((ColorData *) args->node->data)->color;
}

ClientNodeDef client_node_def[COUNT_NODE] = {
	// unknown
	{
		.tiles = TILES_SIMPLE(ASSET_PATH "textures/unknown.png"),
		.visibility = VISIBILITY_SOLID,
		.render = NULL,
		.pointable = true,
		.selection_color = {1.0f, 1.0f, 1.0f},
		.name = "Unknown",
	},
	// air
	{
		.tiles = TILES_NONE,
		.visibility = VISIBILITY_NONE,
		.render = NULL,
		.pointable = false,
		.selection_color = {1.0f, 1.0f, 1.0f},
		.name = "Air",
	},
	// grass
	{
		.tiles = TILES_SIMPLE(ASSET_PATH "textures/grass.png"),
		.visibility = VISIBILITY_SOLID,
		.render = &render_grass,
		.pointable = true,
		.selection_color = {1.0f, 1.0f, 1.0f},
		.name = "Grass",
	},
	// dirt
	{
		.tiles = TILES_SIMPLE(ASSET_PATH "textures/dirt.png"),
		.visibility = VISIBILITY_SOLID,
		.render = NULL,
		.pointable = true,
		.selection_color = {1.0f, 1.0f, 1.0f},
		.name = "Dirt",
	},
	// stone
	{
		.tiles = {
			.paths = { ASSET_PATH "textures/stone.png" },
			.indices = {},
			.textures = {},
			.x4 = { true },
		},
		.visibility = VISIBILITY_SOLID,
		.render = &render_stone,
		.pointable = true,
		.selection_color = {1.0f, 1.0f, 1.0f},
		.name = "Stone",
	},
	// snow
	{
		.tiles = TILES_SIMPLE(ASSET_PATH "textures/snow.png"),
		.visibility = VISIBILITY_SOLID,
		.render = NULL,
		.pointable = true,
		.selection_color = {0.1f, 0.5f, 1.0f},
		.name = "Snow",
	},
	// oak wood
	{
		.tiles = {
			.paths = {ASSET_PATH "textures/oak_wood.png", ASSET_PATH "textures/oak_wood_top.png", NULL, NULL, NULL, NULL},
			.indices = {0, 0, 0, 0, 1, 1},
			.textures = {},
		},
		.visibility = VISIBILITY_SOLID,
		.render = &render_color,
		.pointable = true,
		.selection_color = {1.0f, 1.0f, 1.0f},
		.name = "Oak Wood",
	},
	// oak leaves
	{
		.tiles = TILES_SIMPLE(ASSET_PATH "textures/oak_leaves.png"),
		.visibility = VISIBILITY_SOLID,
		.render = &render_color,
		.pointable = true,
		.selection_color = {1.0f, 1.0f, 1.0f},
		.name = "Oak Leaves",
	},
	// pine wood
	{
		.tiles = {
			.paths = {ASSET_PATH "textures/pine_wood.png", ASSET_PATH "textures/pine_wood_top.png", NULL, NULL, NULL, NULL},
			.indices = {0, 0, 0, 0, 1, 1},
			.textures = {},
		},
		.visibility = VISIBILITY_SOLID,
		.render = &render_color,
		.pointable = true,
		.selection_color = {1.0f, 1.0f, 1.0f},
		.name = "Pine Wood",
	},
	// pine leaves
	{
		.tiles = TILES_SIMPLE(ASSET_PATH "textures/pine_leaves.png"),
		.visibility = VISIBILITY_CLIP,
		.render = &render_color,
		.pointable = true,
		.selection_color = {1.0f, 1.0f, 1.0f},
		.name = "Pine Leaves",
	},
	// palm wood
	{
		.tiles = {
			.paths = {ASSET_PATH "textures/palm_wood.png", ASSET_PATH "textures/palm_wood_top.png", NULL, NULL, NULL, NULL},
			.indices = {0, 0, 0, 0, 1, 1},
			.textures = {},
		},
		.visibility = VISIBILITY_SOLID,
		.render = &render_color,
		.pointable = true,
		.selection_color = {1.0f, 1.0f, 1.0f},
		.name = "Palm Wood",
	},
	// palm leaves
	{
		.tiles = TILES_SIMPLE(ASSET_PATH "textures/palm_leaves.png"),
		.visibility = VISIBILITY_SOLID,
		.render = &render_color,
		.pointable = true,
		.selection_color = {1.0f, 1.0f, 1.0f},
		.name = "Palm Leaves",
	},
	// sand
	{
		.tiles = TILES_SIMPLE(ASSET_PATH "textures/sand.png"),
		.visibility = VISIBILITY_SOLID,
		.render = NULL,
		.pointable = true,
		.selection_color = {1.0f, 1.0f, 1.0f},
		.name = "Sand",
	},
	// water
	{
		.tiles = TILES_SIMPLE(ASSET_PATH "textures/water.png"),
		.visibility = VISIBILITY_BLEND,
		.render = NULL,
		.pointable = false,
		.selection_color = {1.0f, 1.0f, 1.0f},
		.name = "Water",
	},
	// lava
	{
		.tiles = TILES_SIMPLE(ASSET_PATH "textures/lava.png"),
		.visibility = VISIBILITY_BLEND,
		.render = NULL,
		.pointable = false,
		.selection_color = {1.0f, 1.0f, 1.0f},
		.name = "Lava",
	},
	// vulcano_stone
	{
		.tiles = TILES_SIMPLE(ASSET_PATH "textures/vulcano_stone.png"),
		.visibility = VISIBILITY_SOLID,
		.render = NULL,
		.pointable = true,
		.selection_color = {1.0f, 1.0f, 1.0f},
		.name = "Vulcano Stone",
	},
};

Texture client_node_atlas;

void client_node_init()
{
	TextureAtlas atlas = texture_atlas_create(
		client_config.atlas_size, client_config.atlas_size, 4,
		(client_config.atlas_mipmap && client_config.mipmap) ? client_config.atlas_mipmap : 1);

	for (NodeType node = 0; node < COUNT_NODE; node++) {
		ClientNodeDef *def = &client_node_def[node];

		if (def->visibility != VISIBILITY_NONE) {
			TextureSlice textures[6];

			for (int i = 0; i < 6; i++) {
				char *path = def->tiles.paths[i];

				if (path)
					textures[i] = texture_atlas_add(&atlas, path, def->tiles.x4[i]);
				else
					break;
			}

			for (int i = 0; i < 6; i++)
				def->tiles.textures[i] = textures[def->tiles.indices[i]];
		}
	}

	client_node_atlas = texture_atlas_upload(&atlas);
}

void client_node_deinit()
{
	texture_destroy(&client_node_atlas);
}

void client_node_delete(TerrainNode *node)
{
	switch (node->type) {
		NODES_TREE
			free(node->data);
			break;

		default:
			break;
	}
}

void client_node_deserialize(TerrainNode *node, Blob buffer)
{
	switch (node->type) {
		NODES_TREE
			ColorData_read(&buffer, node->data = malloc(sizeof(ColorData)));
			break;

		default:
			break;
	}
}
