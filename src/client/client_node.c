#include "client/client.h"
#include "client/client_node.h"
#include "environment.h"
#include "node.h"
#include "perlin.h"
#include "util.h"
#define TILES_SIMPLE(path) {.paths = {path, NULL, NULL, NULL, NULL, NULL}, .indices = {0, 0, 0, 0, 0, 0}, .textures = {NULL}}
#define TILES_NONE {.paths = {NULL}, .indices = {0}, .textures = {NULL}}

static f64 clamp(f64 v, f64 min, f64 max)
{
	return v < min ? min : v > max ? max : v;
}

static void render_grass(v3s32 pos, unused MapNode *node, Vertex3D *vertex, unused int f, unused int v)
{
	f32 wet_min, wet_max, temp_max;
	wet_min = 0.13f;
	wet_max = 0.33f;
	temp_max = 0.45f;

	f32 temp_f = clamp(0.3f - get_temperature(pos), 0.0f, 0.3f) / 0.3f;

	vertex->color.h = (get_wetness(pos) * (wet_max - wet_min) + wet_min) * (1.0f - temp_f) + temp_max * temp_f;
	vertex->color.s = 1.0f;
	vertex->color.v = 1.0f;
}

static void render_stone(v3s32 pos, unused MapNode *node, Vertex3D *vertex, unused int f, unused int v)
{
	vertex->textureCoordinates.s += smooth2d(U32(pos.x), U32(pos.z), 0, seed + SO_TEXTURE_OFFSET_S);
	vertex->textureCoordinates.t += smooth2d(U32(pos.x), U32(pos.z), 0, seed + SO_TEXTURE_OFFSET_T);
}

static void render_wood(unused v3s32 pos, unused MapNode *node, Vertex3D *vertex, int f, unused int v)
{
	vertex->color.h = f < 4 ? 0.1f : 0.11f;
	vertex->color.s = 1.0f;
	vertex->color.v = 1.0f;
}

ClientNodeDefintion client_node_definitions[NODE_UNLOADED] = {
	// invalid
	{
		.tiles = TILES_SIMPLE(RESSOURCEPATH "textures/invalid.png"),
		.visibility = NV_SOLID,
		.render = NULL,
	},
	// air
	{
		.tiles = TILES_NONE,
		.visibility = NV_NONE,
		.render = NULL,
	},
	// grass
	{
		.tiles = TILES_SIMPLE(RESSOURCEPATH "textures/grass.png"),
		.visibility = NV_SOLID,
		.render = &render_grass,
	},
	// dirt
	{
		.tiles = TILES_SIMPLE(RESSOURCEPATH "textures/dirt.png"),
		.visibility = NV_SOLID,
		.render = NULL,
	},
	// stone
	{
		.tiles = TILES_SIMPLE(RESSOURCEPATH "textures/stone.png"),
		.visibility = NV_SOLID,
		.render = &render_stone,
	},
	// snow
	{
		.tiles = TILES_SIMPLE(RESSOURCEPATH "textures/snow.png"),
		.visibility = NV_SOLID,
		.render = NULL,
	},
	// wood
	{
		.tiles = {
			.paths = {RESSOURCEPATH "textures/wood.png", RESSOURCEPATH "textures/wood_top.png", NULL, NULL, NULL, NULL},
			.indices = {0, 0, 0, 0, 1, 1},
			.textures = {NULL},
		},
		.visibility = NV_SOLID,
		.render = &render_wood,
	},
	// sand
	{
		.tiles = TILES_SIMPLE(RESSOURCEPATH "textures/sand.png"),
		.visibility = NV_SOLID,
		.render = NULL,
	},
	// water
	{
		.tiles = TILES_SIMPLE(RESSOURCEPATH "textures/water.png"),
		.visibility = NV_TRANSPARENT,
		.render = NULL,
	},
};

void client_node_init()
{
	for (Node node = NODE_INVALID; node < NODE_UNLOADED; node++) {
		ClientNodeDefintion *def = &client_node_definitions[node];

		if (client_node_definitions[node].visibility != NV_NONE) {
			Texture *textures[6];

			for (int i = 0; i < 6; i++) {
				char *path = def->tiles.paths[i];

				if (path)
					textures[i] = texture_get(path);
				else
					break;
			}

			for (int i = 0; i < 6; i++)
				def->tiles.textures[i] = textures[def->tiles.indices[i]];
		}
	}
}
