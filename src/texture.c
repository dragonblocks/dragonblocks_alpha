#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <stdbool.h>
#include "list.h"
#include "texture.h"

static List textures;

__attribute((constructor(101))) static void init_textures()
{
	stbi_set_flip_vertically_on_load(true);

	textures = list_create(&list_compare_string);
}

static void list_delete_texture(__attribute__((unused)) void *key, void *value, __attribute__((unused)) void *unused)
{
	Texture *texture = value;
	glDeleteTextures(1, &texture->id);
	free(texture);
}

__attribute((destructor)) static void delete_textures()
{
	list_clear_func(&textures, &list_delete_texture, NULL);
}

static void *create_texture(void *key)
{
	Texture *texture = malloc(sizeof(Texture));
	int channels;

	unsigned char *data = stbi_load(key, &texture->width, &texture->height, &channels, 0);
	if (! data) {
		printf("Failed to load texture %s\n", (char *) key);
		return 0;
	}

	glGenTextures(1, &texture->id);

	glBindTexture(GL_TEXTURE_2D, texture->id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->width, texture->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(data);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texture;
}

Texture *get_texture(char *path)
{
	return list_get_cached(&textures, path, &create_texture);
}
