#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <stdbool.h>
#include <dragonstd/list.h>
#include "client/client_config.h"
#include "client/texture.h"
#include "util.h"

static List textures;

__attribute((constructor(101))) static void textures_init()
{
	textures = list_create(&list_compare_string);
}

static void list_delete_texture(unused void *key, void *value, unused void *arg)
{
	texture_delete(value);
}

__attribute((destructor)) static void textures_deinit()
{
	list_clear_func(&textures, &list_delete_texture, NULL);
}

Texture *texture_create(unsigned char *data, int width, int height, GLenum format, bool mipmap)
{
	Texture *texture = malloc(sizeof(Texture));
	texture->width = width;
	texture->height = height;

	glGenTextures(1, &texture->id);

	glBindTexture(GL_TEXTURE_2D, texture->id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mipmap && client_config.mipmap) ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, format, texture->width, texture->height, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texture;
}

GLuint texture_create_cubemap(char *path)
{
	GLuint id;
	glGenTextures(1, &id);

	glBindTexture(GL_TEXTURE_CUBE_MAP, id);

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

		int width, height, channels;
		unsigned char *data = stbi_load(filename, &width, &height, &channels, 0);
		if (! data) {
			fprintf(stderr, "Failed to load texture %s\n", filename);
			return 0;
		}

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		stbi_image_free(data);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return id;
}

void texture_delete(Texture *texture)
{
	glDeleteTextures(1, &texture->id);
	free(texture);
}

Texture *texture_load(char *path, bool mipmap)
{
	int width, height, channels;

	unsigned char *data = stbi_load(path, &width, &height, &channels, 0);
	if (! data) {
		fprintf(stderr, "Failed to load texture %s\n", path);
		return NULL;
	}

	Texture *texture = texture_create(data, width, height, GL_RGBA, mipmap);

	stbi_image_free(data);

	list_put(&textures, texture, NULL);

	return texture;
}
