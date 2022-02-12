#include <string.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "client/client.h"
#include "client/font.h"
#include "client/texture.h"

#define NUM_CHARS 128

typedef struct
{
	Texture *texture;
	v2s32 bearing;
	u32 advance;
} Character;

static struct
{
	FT_Library library;
	FT_Face face;
	Character chars[NUM_CHARS];
	GLfloat height;
} font;

typedef struct
{
	GLfloat x, y;
} __attribute__((packed)) VertexFontPosition;

typedef struct
{
	GLfloat s, t;
} __attribute__((packed)) VertexFontTextureCoordinates;

typedef struct
{
	VertexFontPosition position;
	VertexFontTextureCoordinates textureCoordinates;
} __attribute__((packed)) VertexFont;

static VertexAttribute vertex_attributes[2] = {
	// position
	{
		.type = GL_FLOAT,
		.length = 2,
		.size = sizeof(VertexFontPosition),
	},
	// textureCoordinates
	{
		.type = GL_FLOAT,
		.length = 2,
		.size = sizeof(VertexFontTextureCoordinates),
	},
};

static VertexLayout vertex_layout = {
	.attributes = vertex_attributes,
	.count = 2,
	.size = sizeof(VertexFont),
};

bool font_init()
{
	if (FT_Init_FreeType(&font.library)) {
		fprintf(stderr, "Failed to initialize Freetype\n");
		return false;
	}

	if (FT_New_Face(font.library, RESSOURCE_PATH "fonts/Minecraftia.ttf", 0, &font.face)) {
		fprintf(stderr, "Failed to load Minecraftia.ttf\n");
		return false;
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	FT_Set_Pixel_Sizes(font.face, 0, 16);

	for (unsigned char c = 0; c < NUM_CHARS; c++) {
		if (FT_Load_Char(font.face, c, FT_LOAD_RENDER)) {
			fprintf(stderr, "Failed to load glyph %c\n", c);

			font.chars[c] = (Character) {
				.texture = NULL,
				.bearing = {0, 0},
				.advance = 0,
			};
		} else {
			font.chars[c] = (Character) {
				.texture = texture_create(font.face->glyph->bitmap.buffer, font.face->glyph->bitmap.width, font.face->glyph->bitmap.rows, GL_RED, false),
				.bearing = {font.face->glyph->bitmap_left, font.face->glyph->bitmap_top},
				.advance = font.face->glyph->advance.x,
			};
		}
	}

	font.height = font.chars['|'].texture->height;

	FT_Done_Face(font.face);
	FT_Done_FreeType(font.library);

	return true;
}

void font_deinit()
{
	for (unsigned char c = 0; c < NUM_CHARS; c++) {
		if (font.chars[c].texture)
			texture_delete(font.chars[c].texture);
	}
}

Font *font_create(const char *text)
{
	Font *fnt = malloc(sizeof(fnt));

	size_t len = strlen(text);

	fnt->meshes = malloc(sizeof(Mesh *) * len);
	fnt->meshes_count = len;

	GLfloat offset = 0.0f;

	for (size_t i = 0; i < len; i++) {
		unsigned char c = text[i];

		if (c >= NUM_CHARS || ! font.chars[c].texture)
			c = '?';

		Character *ch = &font.chars[c];

		GLfloat width = ch->texture->width;
        GLfloat height = ch->texture->height;

		GLfloat x = ch->bearing.x + offset;
        GLfloat y = font.height - ch->bearing.y;

        VertexFont vertices[6] = {
            {{x,         y         }, {0.0f, 0.0f}},
            {{x,         y + height}, {0.0f, 1.0f}},
            {{x + width, y + height}, {1.0f, 1.0f}},
            {{x,         y         }, {0.0f, 0.0f}},
            {{x + width, y + height}, {1.0f, 1.0f}},
            {{x + width, y         }, {1.0f, 0.0f}},
        };

		Mesh *mesh = fnt->meshes[i] = mesh_create();
		mesh->textures = &ch->texture->id;
		mesh->textures_count = 1;
		mesh->free_textures = false;
		mesh->vertices = vertices;
		mesh->vertices_count = 6;
		mesh->free_vertices = false;
		mesh->layout = &vertex_layout;
		mesh_configure(mesh);

		offset += ch->advance >> 6;
	}

	fnt->size = (v2f32) {offset, font.height};

	return fnt;
}

void font_delete(Font *fnt)
{
	for (size_t i = 0; i < fnt->meshes_count; i++)
		mesh_delete(fnt->meshes[i]);

	free(fnt);
}

void font_render(Font *fnt)
{
	for (size_t i = 0; i < fnt->meshes_count; i++)
		mesh_render(fnt->meshes[i]);
}
