#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <stdbool.h>
#include <dragonstd/tree.h>
#include "client/client_config.h"
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
		*texture = &((TextureLookup *) &(*node)->dat)->texture;
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
		exit(EXIT_FAILURE);
	}

	texture_upload(texture, data, GL_RGBA, mipmap);
	stbi_image_free(data);

	return texture;
}

Texture *texture_load_cubemap(char *path)
{
	Texture *texture;
	if (lookup_texture(path, &texture))
		return texture;

	glGenTextures(1, &texture->txo);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture->txo);

	const char *directions[6] = {
		"right",
		"left",
		"top",
		"bottom",
		"front",
		"back",
	};

	for (int i = 0; i < 6; i++) {
		char filename[strlen(path) + 1 + strlen(directions[i]) + 1 + 3 + 1];
		sprintf(filename, "%s/%s.png", path, directions[i]);

		unsigned char *data = stbi_load(filename,
			&texture->width, &texture->height, &texture->channels, 0);
		if (!data) {
			fprintf(stderr, "[error] failed to load texture %s\n", filename);
			exit(EXIT_FAILURE);
		}

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA,
			texture->width, texture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		stbi_image_free(data);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return texture;
}

void texture_upload(Texture *texture, unsigned char *data, GLenum format, bool mipmap)
{
	glGenTextures(1, &texture->txo);
	glBindTexture(GL_TEXTURE_2D, texture->txo);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mipmap && client_config.mipmap)
		? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, format,
		texture->width, texture->height, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
}

void texture_destroy(Texture *texture)
{
	glDeleteTextures(1, &texture->txo);
}
