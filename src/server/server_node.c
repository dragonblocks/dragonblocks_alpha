#include <stdlib.h>
#include "server/server_node.h"

TerrainNode server_node_create(NodeType type)
{
	switch (type) {
		NODES_TREE
			return server_node_create_tree(type, (TreeData) {{0.5f, 0.5f, 0.5f}, 0, {0, 0, 0}});

		default:
			return (TerrainNode) {type, NULL};
	}
}

TerrainNode server_node_create_color(NodeType type, v3f32 color)
{
	switch (type) {
		NODES_TREE
			return server_node_create_tree(type, (TreeData) {color, 0, {0, 0, 0}});

		default:
			return server_node_create(type);
	}
}

TerrainNode server_node_create_tree(NodeType type, TreeData data)
{
	TerrainNode node = {type, malloc(sizeof data)};
	*((TreeData *) node.data) = data;
	return node;
}

TerrainNode server_node_copy(TerrainNode node)
{
	switch (node.type) {
		NODES_TREE
			return server_node_create_tree(node.type, *((TreeData *) node.data));

		default:
			return server_node_create(node.type);
	}
}

void server_node_delete(TerrainNode *node)
{
	switch (node->type) {
		NODES_TREE
			free(node->data);
			break;

		default:
			break;
	}
}

void server_node_deserialize(TerrainNode *node, Blob buffer)
{
	switch (node->type) {
		NODES_TREE
			TreeData_read(&buffer, node->data = malloc(sizeof(TreeData)));
			break;

		default:
			break;
	}
}

void server_node_serialize(TerrainNode *node, Blob *buffer)
{
	switch (node->type) {
		NODES_TREE
			TreeData_write(buffer, node->data);
			break;

		default:
			break;
	}
}

void server_node_serialize_client(TerrainNode *node, Blob *buffer)
{
	switch (node->type) {
		NODES_TREE
			ColorData_write(buffer, &(ColorData) {((TreeData *) node->data)->color});
			break;

		default:
			break;
	}
}
