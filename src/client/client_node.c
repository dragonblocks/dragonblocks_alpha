#include "client/client.h"
#include "client/client_node.h"
#include "environment.h"
#include "node.h"
#include "perlin.h"
#include "util.h"
#define TILES_SIMPLE(path) {.paths = {path, NULL, NULL, NULL, NULL, NULL}, .indices = {0, 0, 0, 0, 0, 0}, .textures = {NULL}}
#define TILES_NONE {.paths = {NULL}, .indices = {0}, .textures = {NULL}}

static f32 hue_to_rgb(f32 p, f32 q, f32 t)
{
    if (t < 0.0f)
        t += 1.0f;

    if (t > 1.0f)
        t -= 1.0f;

    if (t < 1.0f / 6.0f)
        return p + (q - p) * 6.0f * t;

    if (t < 1.0f / 2.0f)
        return q;

    if (t < 2.0f / 3.0f)
        return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;

    return p;
}

static Vertex3DColor hsl_to_rgb(v3f32 hsl)
{
	Vertex3DColor rgb;

    if (hsl.y == 0.0f) {
		rgb = (Vertex3DColor) {hsl.z, hsl.z, hsl.z};
    } else {
        f32 q = hsl.z < 0.5f ? hsl.z * (1.0f + hsl.y) : hsl.z + hsl.y - hsl.z * hsl.y;
        f32 p = 2.0f * hsl.z - q;

        rgb.r = hue_to_rgb(p, q, hsl.x + 1.0f / 3.0f);
        rgb.g = hue_to_rgb(p, q, hsl.x);
        rgb.b = hue_to_rgb(p, q, hsl.x - 1.0f / 3.0f);
    }

    return rgb;
}

static void render_grass(v3s32 pos, unused MapNode *node, Vertex3D *vertex, unused int f, unused int v)
{
	f32 hum_min, hum_max, temp_max;
	hum_min = 0.13f;
	hum_max = 0.33f;
	temp_max = 0.45f;

	f32 temp_f = clamp(0.3f - get_temperature(pos), 0.0f, 0.3f) / 0.3f;

	vertex->color = hsl_to_rgb((v3f32) {(get_humidity(pos) * (hum_max - hum_min) + hum_min) * (1.0f - temp_f) + temp_max * temp_f, 1.0f, 0.5f});
}

static void render_stone(v3s32 pos, unused MapNode *node, Vertex3D *vertex, unused int f, unused int v)
{
	vertex->textureCoordinates.s += noise2d(pos.x, pos.z, 0, seed + SO_TEXTURE_OFFSET_S);
	vertex->textureCoordinates.t += noise2d(pos.x, pos.z, 0, seed + SO_TEXTURE_OFFSET_T);
}

static void render_hsl(unused v3s32 pos, MapNode *node, Vertex3D *vertex, unused int f, unused int v)
{
	vertex->color = hsl_to_rgb(((HSLData *) node->data)->color);
}

ClientNodeDefinition client_node_definitions[NODE_UNLOADED] = {
	// unknown
	{
		.tiles = TILES_SIMPLE(RESSOURCEPATH "textures/unknown.png"),
		.visibility = NV_SOLID,
		.mipmap = true,
		.render = NULL,
	},
	// air
	{
		.tiles = TILES_NONE,
		.visibility = NV_NONE,
		.mipmap = true,
		.render = NULL,
	},
	// grass
	{
		.tiles = TILES_SIMPLE(RESSOURCEPATH "textures/grass.png"),
		.visibility = NV_SOLID,
		.mipmap = true,
		.render = &render_grass,
	},
	// dirt
	{
		.tiles = TILES_SIMPLE(RESSOURCEPATH "textures/dirt.png"),
		.visibility = NV_SOLID,
		.mipmap = true,
		.render = NULL,
	},
	// stone
	{
		.tiles = TILES_SIMPLE(RESSOURCEPATH "textures/stone.png"),
		.visibility = NV_SOLID,
		.mipmap = true,
		.render = &render_stone,
	},
	// snow
	{
		.tiles = TILES_SIMPLE(RESSOURCEPATH "textures/snow.png"),
		.visibility = NV_SOLID,
		.mipmap = true,
		.render = NULL,
	},
	// oak wood
	{
		.tiles = {
			.paths = {RESSOURCEPATH "textures/oak_wood.png", RESSOURCEPATH "textures/oak_wood_top.png", NULL, NULL, NULL, NULL},
			.indices = {0, 0, 0, 0, 1, 1},
			.textures = {NULL},
		},
		.visibility = NV_SOLID,
		.mipmap = true,
		.render = &render_hsl,
	},
	// oak leaves
	{
		.tiles = TILES_SIMPLE(RESSOURCEPATH "textures/oak_leaves.png"),
		.visibility = NV_SOLID,
		.mipmap = true,
		.render = &render_hsl,
	},
	// pine wood
	{
		.tiles = {
			.paths = {RESSOURCEPATH "textures/pine_wood.png", RESSOURCEPATH "textures/pine_wood_top.png", NULL, NULL, NULL, NULL},
			.indices = {0, 0, 0, 0, 1, 1},
			.textures = {NULL},
		},
		.visibility = NV_SOLID,
		.mipmap = true,
		.render = &render_hsl,
	},
	// pine leaves
	{
		.tiles = TILES_SIMPLE(RESSOURCEPATH "textures/pine_leaves.png"),
		.visibility = NV_CLIP,
		.mipmap = true,
		.render = &render_hsl,
	},
	// palm wood
	{
		.tiles = {
			.paths = {RESSOURCEPATH "textures/palm_wood.png", RESSOURCEPATH "textures/palm_wood_top.png", NULL, NULL, NULL, NULL},
			.indices = {0, 0, 0, 0, 1, 1},
			.textures = {NULL},
		},
		.visibility = NV_SOLID,
		.mipmap = true,
		.render = &render_hsl,
	},
	// palm leaves
	{
		.tiles = TILES_SIMPLE(RESSOURCEPATH "textures/palm_leaves.png"),
		.visibility = NV_SOLID,
		.mipmap = true,
		.render = &render_hsl,
	},
	// sand
	{
		.tiles = TILES_SIMPLE(RESSOURCEPATH "textures/sand.png"),
		.visibility = NV_SOLID,
		.mipmap = true,
		.render = NULL,
	},
	// water
	{
		.tiles = TILES_SIMPLE(RESSOURCEPATH "textures/water.png"),
		.visibility = NV_BLEND,
		.mipmap = true,
		.render = NULL,
	},
	// lava
	{
		.tiles = TILES_SIMPLE(RESSOURCEPATH "textures/lava.png"),
		.visibility = NV_BLEND,
		.mipmap = true,
		.render = NULL,
	},
	// vulcano_stone
	{
		.tiles = TILES_SIMPLE(RESSOURCEPATH "textures/vulcano_stone.png"),
		.visibility = NV_SOLID,
		.mipmap = true,
		.render = NULL,
	},
};

void client_node_init()
{
	for (Node node = NODE_UNKNOWN; node < NODE_UNLOADED; node++) {
		ClientNodeDefinition *def = &client_node_definitions[node];

		if (def->visibility != NV_NONE) {
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
