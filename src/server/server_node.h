#ifndef _SERVER_NODE_H_
#define _SERVER_NODE_H_

#include "terrain.h"
#include "types.h"

TerrainNode server_node_create(NodeType type);
TerrainNode server_node_create_color(NodeType type, v3f32 color);
TerrainNode server_node_create_tree(NodeType type, TreeData data);
TerrainNode server_node_copy(TerrainNode node);
void server_node_delete(TerrainNode *node);
void server_node_deserialize(TerrainNode *node, Blob buffer);
void server_node_serialize(TerrainNode *node, Blob *buffer);
void server_node_serialize_client(TerrainNode *node, Blob *buffer);

#endif
