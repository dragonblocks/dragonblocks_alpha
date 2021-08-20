#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <stdbool.h>
#include "client/texture.h"
#include "list.h"
#include "util.h"

static List textures;

__attribute((constructor(101))) static void textures_init()
{
	stbi_set_flip_vertically_on_load(true);

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

Texture *texture_create(unsigned char *data, int width, int height, GLenum format)
{
	Texture *texture = malloc(sizeof(Texture));
	texture->width = width;
	texture->height = height;

	glGenTextures(1, &texture->id);

	glBindTexture(GL_TEXTURE_2D, texture->id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, format, texture->width, texture->height, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texture;
}

void texture_delete(Texture *texture)
{
	glDeleteTextures(1, &texture->id);
	free(texture);
}

static void *create_image_texture(void *key)
{
	int width, height, channels;

	unsigned char *data = stbi_load(key, &width, &height, &channels, 0);
	if (! data) {
		printf("Failed to load texture %s\n", (char *) key);
		return 0;
	}

	Texture *texture = texture_create(data, width, height, GL_RGBA);

	stbi_image_free(data);

	return texture;
}

Texture *texture_get(char *path)
{
	return list_get_cached(&textures, path, &create_image_texture);
}
