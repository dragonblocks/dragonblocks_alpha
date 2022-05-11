#include <dragonstd/tree.h>
#include <getline.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "client/cube.h"
#include "client/opengl.h"
#include "client/mesh.h"

typedef struct {
	v3s32 pos;
	v3f32 color;
} LoadedVoxel;

typedef struct {
	Tree voxels;
	Array vertices;
} LoadedRenderArgs;

static v3s32 face_dir[6] = {
	{+0, +0, -1},
	{+0, +0, +1},
	{-1, +0, +0},
	{+1, +0, +0},
	{+0, -1, +0},
	{+0, +1, +0},
};

typedef struct {
	v3f32 pos;
	v3f32 normal;
	v3f32 color;
} __attribute__((packed)) LoadedVertex;
static VertexLayout loaded_layout = {
	.attributes = (VertexAttribute[]) {
		{GL_FLOAT, 3, sizeof(v3f32)}, // position
		{GL_FLOAT, 3, sizeof(v3f32)}, // normal
		{GL_FLOAT, 3, sizeof(v3f32)}, // color
	},
	.count = 3,
	.size = sizeof(LoadedVertex),
};

static int cmp_loaded_voxel(const LoadedVoxel *voxel, const v3s32 *pos)
{
	return v3s32_cmp(&voxel->pos, pos);
}

static void render_loaded_voxel(LoadedVoxel *voxel, LoadedRenderArgs *args)
{
	v3f32 pos = v3s32_to_f32(voxel->pos);
	for (int f = 0; f < 6; f++) {
		v3s32 neigh_pos = v3s32_add(voxel->pos, face_dir[f]);
		if (tree_get(&args->voxels, &neigh_pos, &cmp_loaded_voxel, NULL))
			continue;

		for (int v = 0; v < 6; v++)
			array_apd(&args->vertices, &(LoadedVertex) {
				v3f32_add(cube_vertices[f][v].position, pos),
				cube_vertices[f][v].normal,
				voxel->color,
			});
	}
}

void mesh_load(Mesh *mesh, const char *path)
{
	mesh->layout = &loaded_layout;
	mesh->vao = mesh->vbo = 0;
	mesh->data = NULL;
	mesh->count = 0;
	mesh->free_data = true;

	LoadedRenderArgs args;
	tree_ini(&args.voxels);
	array_ini(&args.vertices, sizeof(LoadedVertex), 500);

	FILE *file = fopen(path, "r");
	if (!file) {
		fprintf(stderr, "[warning] failed to open mesh %s\n", path);
		return;
	}

	char *line = NULL;
	size_t siz = 0;
	ssize_t length;
	int count = 0;

	while ((length = getline(&line, &siz, file)) > 0) {
		count++;

		if (*line == '#')
			continue;

		LoadedVoxel *voxel = malloc(sizeof *voxel);

		v3s32 color;
		if (sscanf(line, "%d %d %d %2x%2x%2x",
				&voxel->pos.x, &voxel->pos.z, &voxel->pos.y,
				&color.x, &color.y, &color.z) != 6) {
			fprintf(stderr, "[warning] syntax error in mesh %s in line %d: %s\n",
				path, count, line);
			free(voxel);
			continue;
		}

		voxel->color = (v3f32) {
			(f32) color.x / 0xFF,
			(f32) color.y / 0xFF,
			(f32) color.z / 0xFF,
		};

		if (!tree_add(&args.voxels, &voxel->pos, voxel, &cmp_loaded_voxel, NULL)) {
			fprintf(stderr, "[warning] more than one voxel at position (%d, %d, %d) in mesh %s in line %d\n",
				voxel->pos.x, voxel->pos.y, voxel->pos.z, path, count);
			free(voxel);
		}
	}

	if (line)
		free(line);

	fclose(file);

	tree_trv(&args.voxels, &render_loaded_voxel, &args, NULL, 0);
	tree_clr(&args.voxels, &free, NULL, NULL, 0);

	mesh->data = args.vertices.ptr;
	mesh->count = args.vertices.siz;
}

// upload data to GPU (only done once)
void mesh_upload(Mesh *mesh)
{
	glGenVertexArrays(1, &mesh->vao); GL_DEBUG
	glGenBuffers(1, &mesh->vbo); GL_DEBUG

	glBindVertexArray(mesh->vao); GL_DEBUG
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo); GL_DEBUG

	glBufferData(GL_ARRAY_BUFFER, mesh->count * mesh->layout->size,
		mesh->data, GL_STATIC_DRAW); GL_DEBUG

	size_t offset = 0;
	for (GLuint i = 0; i < mesh->layout->count; i++) {
		VertexAttribute *attrib = &mesh->layout->attributes[i];

		glVertexAttribPointer(i, attrib->length, attrib->type, GL_FALSE,
			mesh->layout->size, (GLvoid *) offset); GL_DEBUG
		glEnableVertexAttribArray(i); GL_DEBUG

		offset += attrib->size;
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0); GL_DEBUG
	glBindVertexArray(0); GL_DEBUG

	if (mesh->free_data)
		free(mesh->data);

	mesh->data = NULL;
}

void mesh_render(Mesh *mesh)
{
	if (mesh->data)
		mesh_upload(mesh);

	// render
	glBindVertexArray(mesh->vao); GL_DEBUG
	glDrawArrays(GL_TRIANGLES, 0, mesh->count); GL_DEBUG
}

void mesh_destroy(Mesh *mesh)
{
	if (mesh->data && mesh->free_data)
		free(mesh->data);

	if (mesh->vao) {
		glDeleteVertexArrays(1, &mesh->vao); GL_DEBUG
	}

	if (mesh->vbo) {
		glDeleteBuffers(1, &mesh->vbo); GL_DEBUG
	}

	mesh->vao = mesh->vbo = 0;
}
