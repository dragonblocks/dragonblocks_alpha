#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_resize.h>
#include <stdbool.h>
#include <dragonstd/tree.h>
#include "client/client_config.h"
#include "client/opengl.h"
#include "client/texture.h"

static Tree textures;

typedef struct {
	char *path;
	Texture texture;
} TextureLookup;

static int cmp_texture(TextureLookup *texture, char *path)
{
	return strcmp(texture->path, path);
}

static bool lookup_texture(char *path, Texture **texture)
{
	TreeNode **node = tree_nfd(&textures, path, &cmp_texture);

	if (*node) {
		*texture = &((TextureLookup *) (*node)->dat)->texture;
		return true;
	}

	TextureLookup *lookup = malloc(sizeof *lookup);
	lookup->path = strdup(path);
	*texture = &lookup->texture;

	tree_nmk(&textures, node, lookup);
	return false;
}

static void delete_texture(TextureLookup *lookup)
{
	free(lookup->path);
	texture_destroy(&lookup->texture);
	free(lookup);
}

__attribute__((constructor(101))) static void textures_init()
{
	tree_ini(&textures);
}

__attribute__((destructor)) static void textures_deinit()
{
	tree_clr(&textures, &delete_texture, NULL, NULL, 0);
}

Texture *texture_load(char *path, bool mipmap)
{
	Texture *texture;
	if (lookup_texture(path, &texture))
		return texture;

	unsigned char *data = stbi_load(path,
		&texture->width, &texture->height, &texture->channels, 0);
	if (!data) {
		fprintf(stderr, "[error] failed to load texture %s\n", path);
		abort();
	}

	texture_upload(texture, data, GL_RGBA, mipmap);
	stbi_image_free(data);

	return texture;
}

static inline int least_common_multiple(int a, int b)
{
	int high, low;
	if (a > b) {
		high = a;
		low = b;
	} else {
		high = b;
		low = a;
	}

	int lcm = high;
	while (lcm % low)
		lcm += high;
	return lcm;
}

Texture *texture_load_cubemap(char *path, bool bilinear_filter)
{
	Texture *texture;
	if (lookup_texture(path, &texture))
		return texture;

	glGenTextures(1, &texture->txo); GL_DEBUG
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture->txo); GL_DEBUG

	const char *directions[6] = {
		"right",
		"left",
		"top",
		"bottom",
		"front",
		"back",
	};

	typedef struct {
		unsigned char *data;
		int width, height, channels;
	} CubemapFace;

	CubemapFace faces[6];
	int size = 1;

	for (int i = 0; i < 6; i++) {
		char filename[strlen(path) + 1 + strlen(directions[i]) + 1 + 3 + 1];
		sprintf(filename, "%s/%s.png", path, directions[i]);

		if (!(faces[i].data = stbi_load(filename,
				&faces[i].width, &faces[i].height, &faces[i].channels, 0))) {
			fprintf(stderr, "[error] failed to load texture %s\n", filename);
			abort();
		}

		size = least_common_multiple(size, faces[i].width);
		size = least_common_multiple(size, faces[i].height);
	}

	for (int i = 0; i < 6; i++) {
		unsigned char *data = faces[i].data;

		bool resize = faces[i].width != size || faces[i].height != size;
		if (resize) {
			data = malloc(size * size * faces[i].channels);

			stbir_resize_uint8_generic(
				faces[i].data, faces[i].width, faces[i].height, 0,
				data, size, size, 0,
				faces[i].channels, STBIR_ALPHA_CHANNEL_NONE, 0,
				STBIR_EDGE_CLAMP, STBIR_FILTER_BOX, STBIR_COLORSPACE_LINEAR,
				NULL);
		}

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA,
			size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, data); GL_DEBUG

		stbi_image_free(faces[i].data);
		if (resize)
			stbi_image_free(data);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, bilinear_filter ? GL_LINEAR : GL_NEAREST); GL_DEBUG
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, bilinear_filter ? GL_LINEAR : GL_NEAREST); GL_DEBUG
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); GL_DEBUG
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); GL_DEBUG
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE); GL_DEBUG

	return texture;
}

void texture_upload(Texture *texture, unsigned char *data, GLenum format, bool mipmap)
{
	glGenTextures(1, &texture->txo); GL_DEBUG
	glBindTexture(GL_TEXTURE_2D, texture->txo); GL_DEBUG

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mipmap && client_config.mipmap)
		? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST); GL_DEBUG
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); GL_DEBUG
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); GL_DEBUG
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); GL_DEBUG

	glTexImage2D(GL_TEXTURE_2D, 0, format,
		texture->width, texture->height, 0, format, GL_UNSIGNED_BYTE, data); GL_DEBUG
	glGenerateMipmap(GL_TEXTURE_2D); GL_DEBUG
}

void texture_destroy(Texture *texture)
{
	glDeleteTextures(1, &texture->txo); GL_DEBUG
}

TextureAtlas texture_atlas_create(int width, int height, int channels, size_t mipmap_levels)
{
	TextureAtlas atlas = {
		.texture = { 0, width, height, channels },
		.mipmap_levels = mipmap_levels,
		.data = malloc(sizeof(unsigned char *) * mipmap_levels),
		.alloc = {
			.x = 0,
			.y = 0,
			.row_height = 0,
		},
	};

	for (size_t i = 0; i < atlas.mipmap_levels; i++) {
		atlas.data[i] = calloc(width * height, channels);

		width /= 2;
		height /= 2;

		if (width == 0 || height == 0)
			atlas.mipmap_levels = i;
	}

	return atlas;
}

TextureSlice texture_atlas_add(TextureAtlas *atlas, char *path, bool x4)
{
	int width, height, channels;

	unsigned char *data = stbi_load(path, &width, &height, &channels, 0);
	if (!data) {
		fprintf(stderr, "[error] failed to load texture %s\n", path);
		abort();
	}

	if (x4) {
		int new_width = width * 2, new_height = height * 2;
		unsigned char *new_data = malloc(new_width * new_height * channels);

		for (int x = 0; x < 2; x++) for (int y = 0; y < 2; y++)
			// abusing resize for copy...
			stbir_resize_uint8(
				data, width, height, 0,
				new_data + (y * height * new_width + x * width) * channels,
				width, height, new_width * channels, channels);

		stbi_image_free(data);
		width = new_width;
		height = new_height;
		data = new_data;
	}

	if (channels != atlas->texture.channels) {
		fprintf(stderr, "[error] invalid number of channels in %s, expected %d got %d\n",
			path, atlas->texture.channels, channels);
		abort();
	}

	if (atlas->mipmap_levels > 1 && ((width & (width - 1)) != 0 || (height & (height - 1)) != 0)) {
		fprintf(stderr, "[error] texture %s dimensions are not powers of two\n", path);
		abort();
	}

	if (width > atlas->texture.width || height > atlas->texture.height) {
		fprintf(stderr, "[error] texture %s does not fit into atlas\n", path);
		abort();
	}

	if ((int) atlas->alloc.x + width >= atlas->texture.width) {
		atlas->alloc.x = 0;
		atlas->alloc.y += atlas->alloc.row_height;
		atlas->alloc.row_height = 0;
	}

	if ((int) atlas->alloc.y + height >= atlas->texture.height) {
		fprintf(stderr, "[error] texture %s has no space left in atlas\n", path);
		abort();
	}

	unsigned int x = atlas->alloc.x;
	unsigned int y = atlas->alloc.y;

	atlas->alloc.x += width;
	if ((int) atlas->alloc.row_height < height)
		atlas->alloc.row_height = height;

	for (size_t i = 0; i < atlas->mipmap_levels; i++) {
		unsigned int mm_width = width >> i;
		unsigned int mm_height = height >> i;

		if (mm_width == 0) mm_width = 1;
		if (mm_height == 0) mm_height = 1;

		stbir_resize_uint8(
			data, width, height, 0,
			atlas->data[i] + ((atlas->texture.width >> i) * (y >> i) + (x >> i)) * channels,
			mm_width, mm_height, (atlas->texture.width >> i) * channels,
			channels
		);
	}

	stbi_image_free(data);

	return (TextureSlice) {
		.tex_coord_x = ((GLfloat) x) / ((GLfloat) atlas->texture.width),
		.tex_coord_y = ((GLfloat) y) / ((GLfloat) atlas->texture.height),
		.tex_coord_w = ((GLfloat) width) / ((GLfloat) atlas->texture.width),
		.tex_coord_h = ((GLfloat) height) / ((GLfloat) atlas->texture.height),
		.width = width,
		.height = height,
	};
}

Texture texture_atlas_upload(TextureAtlas *atlas)
{
	glGenTextures(1, &atlas->texture.txo); GL_DEBUG
	glBindTexture(GL_TEXTURE_2D, atlas->texture.txo); GL_DEBUG

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, atlas->mipmap_levels > 1
		? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST); GL_DEBUG
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); GL_DEBUG
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); GL_DEBUG
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); GL_DEBUG

	if (atlas->mipmap_levels > 1) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0); GL_DEBUG
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, atlas->mipmap_levels-1); GL_DEBUG
	}

	for (size_t i = 0; i < atlas->mipmap_levels; i++) {
		glTexImage2D(GL_TEXTURE_2D, i, GL_RGBA, atlas->texture.width >> i, atlas->texture.height >> i,
			0, GL_RGBA, GL_UNSIGNED_BYTE, atlas->data[i]); GL_DEBUG
		stbi_image_free(atlas->data[i]);
	}

	free(atlas->data);
	atlas->data = NULL;

	return atlas->texture;
}
