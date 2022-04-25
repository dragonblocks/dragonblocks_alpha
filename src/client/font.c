#include <string.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "client/client.h"
#include "client/font.h"
#include "client/gl_debug.h"
#include "client/texture.h"

#define NUM_CHARS 128

typedef struct {
	Texture texture;
	v2s32 bearing;
	u32 advance;
} Character;

static FT_Library font_library;
static FT_Face font_face;
static Character font_chars[NUM_CHARS];
static GLfloat font_height;

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
	if (FT_Init_FreeType(&font_library)) {
		fprintf(stderr, "[error] failed to initialize Freetype\n");
		abort();
	}

	if (FT_New_Face(font_library, ASSET_PATH "fonts/Minecraftia.ttf", 0, &font_face)) {
		fprintf(stderr, "[error] failed to load Minecraftia.ttf\n");
		abort();
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); GL_DEBUG
	FT_Set_Pixel_Sizes(font_face, 0, 16);

	for (unsigned char c = 0; c < NUM_CHARS; c++) {
		if (FT_Load_Char(font_face, c, FT_LOAD_RENDER)) {
			fprintf(stderr, "[warning] failed to load glyph %c\n", c);
			font_chars[c].texture.txo = 0;
			continue;
		}

		font_chars[c].texture.width = font_face->glyph->bitmap.width;
		font_chars[c].texture.height = font_face->glyph->bitmap.rows;
		texture_upload(&font_chars[c].texture, font_face->glyph->bitmap.buffer, GL_RED, false);

		font_chars[c].bearing = (v2s32) {font_face->glyph->bitmap_left, font_face->glyph->bitmap_top};
		font_chars[c].advance = font_face->glyph->advance.x;
	}

	font_height = font_chars['|'].texture.height;

	FT_Done_Face(font_face);
	FT_Done_FreeType(font_library);
}

void font_deinit()
{
	for (unsigned char c = 0; c < NUM_CHARS; c++)
		texture_destroy(&font_chars[c].texture);
}

Font *font_create(const char *text)
{
	Font *font = malloc(sizeof *font);

	font->count = strlen(text);
	font->meshes = malloc(font->count * sizeof *font->meshes);
	font->textures = malloc(font->count * sizeof *font->textures);

	GLfloat offset = 0.0f;

	for (size_t i = 0; i < font->count; i++) {
		unsigned char c = text[i];

		if (c >= NUM_CHARS || !font_chars[c].texture.txo)
			c = '?';

		Character *ch = &font_chars[c];

		GLfloat width = ch->texture.width;
		GLfloat height = ch->texture.height;

		GLfloat x = ch->bearing.x + offset;
		GLfloat y = font_height - ch->bearing.y;

		// this is art
		// selling this as NFT starting price is 10 BTC
		font->meshes[i].data = (FontVertex[]) {
			{{x,         y         }, {0.0f, 0.0f}},
			{{x,         y + height}, {0.0f, 1.0f}},
			{{x + width, y + height}, {1.0f, 1.0f}},
			{{x,         y         }, {0.0f, 0.0f}},
			{{x + width, y + height}, {1.0f, 1.0f}},
			{{x + width, y         }, {1.0f, 0.0f}},
		};
		font->meshes[i].count = 6;
		font->meshes[i].layout = &font_vertex_layout;
		font->meshes[i].vao = font->meshes[i].vbo = 0;
		font->meshes[i].free_data = false;
		mesh_upload(&font->meshes[i]);

		font->textures[i] = ch->texture.txo;

		offset += ch->advance >> 6;
	}

	font->size = (v2f32) {offset, font_height};

	return font;
}

void font_delete(Font *font)
{
	for (size_t i = 0; i < font->count; i++)
		mesh_destroy(&font->meshes[i]);

	free(font->meshes);
	free(font->textures);
	free(font);
}

void font_render(Font *font)
{
	for (size_t i = 0; i < font->count; i++) {
		glBindTextureUnit(0, font->textures[i]); GL_DEBUG
		mesh_render(&font->meshes[i]);
	}
}
