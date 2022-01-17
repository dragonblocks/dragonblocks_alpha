#ifndef _MAPGEN_H_
#define _MAPGEN_H_

#include "map.h"
#include "server/server_map.h"

void mapgen_set_node(v3s32 pos, MapNode node, MapgenStage mgs, List *changed_blocks);
void mapgen_generate_block(MapBlock *block, List *changed_blocks);                    // generate a block (does not manage block state or threading)

#endif
