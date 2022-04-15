#ifndef _CLIENT_NODE_H_
#define _CLIENT_NODE_H_

#include "client/terrain_gfx.h"
#include "client/texture.h"
#include "terrain.h"

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
		char *paths[6];       // input
		int indices[6];       // input
		Texture *textures[6]; // output
	} tiles;
	NodeVisibility visibility;
	bool mipmap;
	void (*render)(NodeArgsRender *args);
} ClientNodeDefinition;

extern ClientNodeDefinition client_node_definitions[];
void client_node_init();

#endif // _CLIENT_NODE_H_
