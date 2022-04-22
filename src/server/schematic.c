#include <stdio.h>
#include <stdlib.h>
#include "server/schematic.h"
#include "server/server_node.h"
#include "terrain.h"

void schematic_load(List *schematic, const char *path, SchematicMapping *mappings, size_t num_mappings)
{
	list_ini(schematic);

	FILE *file = fopen(path, "r");
	if (!file) {
		fprintf(stderr, "[warning] failed to open schematic %s\n", path);
		return;
	}

	char *line = NULL;
	size_t siz = 0;
	ssize_t length;
	int count = 0;

	// getline is POSIX 2008, so we can use it
	while ((length = getline(&line, &siz, file)) > 0) {
		count++;

		if (*line == '#')
			continue;

		SchematicNode *node = malloc(sizeof *node);

		v3s32 color;
		if (sscanf(line, "%d %d %d %2x%2x%2x",
				&node->pos.x, &node->pos.z, &node->pos.y,
				&color.x, &color.y, &color.z) != 6) {
			fprintf(stderr, "[warning] syntax error in schematic %s in line %d: %s\n",
				path, count, line);
			free(node);
			continue;
		}

		SchematicMapping *mapping = NULL;
		for (size_t i = 0; i < num_mappings; i++)
			if (v3s32_equals(color, mappings[i].color)) {
				mapping = &mappings[i];
				break;
			}

		if (!mapping) {
			fprintf(stderr, "[warning] color not mapped to node in schematic %s in line %d: %02x%02x%02x\n",
				path, count, color.x, color.y, color.z);
			free(node);
			continue;
		}

		node->node = mapping->use_color
			? server_node_create_color(mapping->type, (v3f32) {
				(f32) color.x / 0xFF,
				(f32) color.y / 0xFF,
				(f32) color.z / 0xFF,
			}) : server_node_create(mapping->type);

		list_apd(schematic, node);
	}

	if (line)
		free(line);

	fclose(file);
}

void schematic_place(List *schematic, v3s32 pos, TerrainGenStage tgs, List *changed_chunks)
{
	LIST_ITERATE(schematic, list_node) {
		SchematicNode *node = list_node->dat;

		server_terrain_gen_node(
			v3s32_add(pos, node->pos),
			server_node_copy(node->node),
			tgs, changed_chunks);
	}
}

static void delete_schematic_node(SchematicNode *node)
{
	server_node_delete(&node->node);
	free(node);
}

void schematic_delete(List *schematic)
{
	list_clr(schematic, &delete_schematic_node, NULL, NULL);
}
