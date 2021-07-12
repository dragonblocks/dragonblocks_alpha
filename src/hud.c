#include <string.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include "client.h"
#include "cube.h"
#include "hud.h"

static struct
{
	MeshObject *obj;
	List elements;
	ShaderProgram *prog;
	mat4x4 view, projection;
	int width, height;
} hud;

void hud_init(ShaderProgram *prog)
{
	hud.prog = prog;
	hud.elements = list_create(NULL);

	Texture *texture = get_texture(RESSOURCEPATH "textures/invalid.png");
	VertexBuffer buffer = vertexbuffer_create();
	vertexbuffer_set_texture(&buffer, texture);

	for (int v = 0; v < 6; v++) {
		Vertex vertex = cube_vertices[0][v];
		vertex.z = 0.0f;
		vertexbuffer_add_vertex(&buffer, &vertex);
	}

	hud.obj = meshobject_create(buffer, NULL, (v3f) {0.0f, 0.0f, 0.0f});

	mat4x4_identity(hud.view);
}

static void free_element(void *key, __attribute__((unused)) void *value, __attribute__((unused)) void *arg)
{
	free(key);
}

void hud_deinit()
{
	meshobject_delete(hud.obj);
	list_clear_func(&hud.elements, &free_element, NULL);
}

static void element_transform(HUDElement *element)
{
	mat4x4_translate(element->transform, (1.0f + element->pos.x) / 2.0f * (f32) hud.width, (1.0f + element->pos.y) / 2.0f * (f32) hud.height, 0.0f);

	v2f scale = element->scale;

	switch (element->scale_type) {
		case HUD_SCALE_TEXTURE:
			scale.x *= element->texture->width;
			scale.y *= element->texture->height;

			break;

		case HUD_SCALE_SCREEN:
			scale.x *= hud.width * 2.0f;
			scale.y *= hud.height * 2.0f;

			break;

		default:
			break;
	}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
	mat4x4_scale_aniso(element->transform, element->transform, scale.x, scale.y, 1.0f);
#pragma GCC diagnostic pop
}

void hud_rescale(int width, int height)
{
	hud.width = width;
	hud.height = height;
	mat4x4_ortho(hud.projection, 0, width, height, 0, -1.0f, 1.0f);

	for (ListPair *pair = hud.elements.first; pair != NULL; pair = pair->next)
		element_transform(pair->key);
}

void hud_render()
{
	glDisable(GL_DEPTH_TEST);

	glUniformMatrix4fv(hud.prog->loc_view, 1, GL_FALSE, hud.view[0]);
	glUniformMatrix4fv(hud.prog->loc_projection, 1, GL_FALSE, hud.projection[0]);

	for (ListPair *pair = hud.elements.first; pair != NULL; pair = pair->next) {
		HUDElement *element = pair->key;

		if (element->visible) {
			memcpy(hud.obj->transform, element->transform, sizeof(mat4x4));
			hud.obj->meshes[0]->texture = element->texture->id;
			meshobject_render(hud.obj, hud.prog);
		}
	}
}

HUDElement *hud_add(char *texture, v2f pos, v2f scale, HUDScaleType scale_type)
{
	HUDElement *element = malloc(sizeof(HUDElement));
	element->texture = get_texture(texture);
	element->visible = true;
	element->pos = pos;
	element->scale = scale;
	element->scale_type = scale_type;

	element_transform(element);

	list_set(&hud.elements, element, NULL);
	return element;
}
