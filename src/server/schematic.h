#ifndef _SCHEMATIC_H_
#define _SCHEMATIC_H_

#include <dragonstd/list.h>
#include <stdbool.h>
#include <stddef.h>
#include "node.h"
#include "server/server_terrain.h"
#include "types.h"

typedef struct {
	v3s32 color;
	NodeType type;
	bool use_color;
} SchematicMapping;

typedef struct {
	v3s32 pos;
	TerrainNode node;
} SchematicNode;

void schematic_load(List *schematic, const char *path, SchematicMapping *mappings, size_t num_mappings);
void schematic_place(List *schematic, v3s32 pos, TerrainGenStage tgs, List *changed_chunks);
void schematic_delete(List *schematic);

#endif // _SCHEMATIC_H_
