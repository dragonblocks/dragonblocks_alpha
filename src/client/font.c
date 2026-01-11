#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "client/client.h"
#include "client/font.h"
#include "client/opengl.h"
#include "client/texture.h"
#include "common/fs.h"

#define NUM_CHARS 128

static stbtt_bakedchar font_chars[NUM_CHARS];
static Texture font_atlas;
static unsigned int font_atlas_size = 512;

typedef struct {
	v2f32 position;
	v2f32 textureCoordinates;
} __attribute__((packed)) FontVertex;
static VertexLayout font_vertex_layout = {
	.attributes = (VertexAttribute[]) {
		{GL_FLOAT, 2, sizeof(v2f32)}, // position
		{GL_FLOAT, 2, sizeof(v2f32)}, // textureCoordinates
	},
	.count = 2,
	.size = sizeof(FontVertex),
};

void font_init()
{
	unsigned char *ttf = (unsigned char *) read_file(ASSET_PATH "fonts/Minecraftia.ttf");
	if (!ttf) {
		fprintf(stderr, "[error] could not load Minecraftia.ttf\n");
		abort();
	}

	unsigned char font_atlas_data[font_atlas_size*font_atlas_size];
	stbtt_BakeFontBitmap(ttf, 0, 24.0, font_atlas_data,
		font_atlas_size, font_atlas_size, 0, NUM_CHARS, font_chars);

	font_atlas.width = font_atlas_size;
	font_atlas.height = font_atlas_size;
	font_atlas.channels = 1;

	texture_upload(&font_atlas, font_atlas_data, GL_RED, false);

	free(ttf);
}

void font_deinit()
{
	texture_destroy(&font_atlas);
}

Font *font_create(const char *text)
{
	size_t len = strlen(text);

	FontVertex vertices[len][6];
	GLfloat x = 0.0, y = 14.0;
	for (size_t i = 0; i < len; i++) {
		stbtt_aligned_quad q;
		stbtt_GetBakedQuad(font_chars, font_atlas_size, font_atlas_size,
			text[i], &x, &y, &q, 1);

		FontVertex verts[] = {
			{ { q.x0, q.y0 }, { q.s0, q.t0 } },
			{ { q.x1, q.y1 }, { q.s1, q.t1 } },
			{ { q.x1, q.y0 }, { q.s1, q.t0 } },
			{ { q.x1, q.y1 }, { q.s1, q.t1 } },
			{ { q.x0, q.y0 }, { q.s0, q.t0 } },
			{ { q.x0, q.y1 }, { q.s0, q.t1 } },
		};

		memcpy(&vertices[i], verts, sizeof verts);
	}

	Font *font = malloc(sizeof *font);
	font->mesh.count = len * 6;
	font->mesh.layout = &font_vertex_layout;
	font->mesh.vao = font->mesh.vbo = 0;
	font->mesh.data = &vertices[0][0];
	font->mesh.free_data = false;
	mesh_upload(&font->mesh);

	font->size = (v2f32) { x, y };

	return font;
}

void font_delete(Font *font)
{
	mesh_destroy(&font->mesh);
	free(font);
}

void font_render(Font *font)
{
	opengl_bind_texture(GL_TEXTURE_2D, 0, font_atlas.txo);
	mesh_render(&font->mesh);
}
