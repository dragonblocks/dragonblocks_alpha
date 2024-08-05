#ifndef _CLIENT_NODE_H_
#define _CLIENT_NODE_H_

#include "client/terrain_gfx.h"
#include "client/texture.h"
#include "common/terrain.h"

typedef enum {
	VISIBILITY_NONE,
	VISIBILITY_CLIP,
	VISIBILITY_BLEND,
	VISIBILITY_SOLID,
} NodeVisibility;

typedef struct {
	v3s32 pos;
	TerrainNode *node;
	TerrainVertex vertex;
	unsigned int f, v;
} NodeArgsRender;

typedef struct {
	struct {
		char *paths[6];           // input
		int indices[6];           // input
		bool x4[6];               // input
		TextureSlice textures[6]; // output
	} tiles;
	NodeVisibility visibility;
	void (*render)(NodeArgsRender *args);
	bool pointable;
	v3f32 selection_color;
	char *name;
} ClientNodeDef;

extern ClientNodeDef client_node_def[];
extern Texture client_node_atlas;

void client_node_init();
void client_node_deinit();

void client_node_delete(TerrainNode *node);
void client_node_deserialize(TerrainNode *node, Blob buffer);

#endif // _CLIENT_NODE_H_
