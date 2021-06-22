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
	glDeleteTextures(1, value);
	free(value);
}

__attribute((destructor)) static void delete_textures()
{
	list_clear_func(&textures, &list_delete_texture, NULL);
}

static void *create_texture(void *key)
{
	int width, height, channels;

	unsigned char *data = stbi_load(key, &width, &height, &channels, 0);
	if (! data) {
		printf("Failed to load texture %s\n", (char *) key);
		return 0;
	}

	GLuint *id = malloc(sizeof(GLuint));

	glGenTextures(1, id);

	glBindTexture(GL_TEXTURE_2D, *id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(data);

	glBindTexture(GL_TEXTURE_2D, 0);

	return id;
}

GLuint get_texture(char *path)
{
	return *(GLuint *)list_get_cached(&textures, path, &create_texture);
}
