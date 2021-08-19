#ifndef _MAPGEN_H_
#define _MAPGEN_H_

#include "map.h"

void mapgen_generate_block(MapBlock *block, List *changed_blocks);	// generate a block (does not manage block state or threading)

#endif
