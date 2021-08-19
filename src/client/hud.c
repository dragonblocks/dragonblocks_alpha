#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include "client/client.h"
#include "client/cube.h"
#include "client/hud.h"
#include "client/mesh.h"
#include "client/shader.h"
#include "client/vertex.h"

static struct
{
	List elements;

	GLuint image_prog;
	GLint image_loc_model;
	GLint image_loc_projection;
	Mesh *image_mesh;

	GLuint font_prog;
	GLint font_loc_model;
	GLint font_loc_projection;
	GLint font_loc_text_color;

	mat4x4 projection;
	int width, height;
} hud;

typedef struct
{
	GLfloat x, y;
} __attribute__((packed)) VertexImagePosition;

typedef struct
{
	GLfloat s, t;
} __attribute__((packed)) VertexImageTextureCoordinates;

typedef struct
{
	VertexImagePosition position;
	VertexImageTextureCoordinates textureCoordinates;
} __attribute__((packed)) VertexImage;

static VertexAttribute image_vertex_attributes[2] = {
	// position
	{
		.type = GL_FLOAT,
		.length = 2,
		.size = sizeof(VertexImagePosition),
	},
	// textureCoordinates
	{
		.type = GL_FLOAT,
		.length = 2,
		.size = sizeof(VertexImageTextureCoordinates),
	},
};

static VertexLayout image_vertex_layout = {
	.attributes = image_vertex_attributes,
	.count = 2,
	.size = sizeof(VertexImage),
};

static VertexImage image_vertices[6] = {
	{{-0.5, -0.5}, {+0.0, +0.0}},
	{{+0.5, -0.5}, {+1.0, +0.0}},
	{{+0.5, +0.5}, {+1.0, +1.0}},
	{{+0.5, +0.5}, {+1.0, +1.0}},
	{{-0.5, +0.5}, {+0.0, +1.0}},
	{{-0.5, -0.5}, {+0.0, +0.0}},
};

bool hud_init()
{
	if (! shader_program_create(RESSOURCEPATH "shaders/hud/image", &hud.image_prog)) {
		fprintf(stderr, "Failed to create HUD image shader program\n");
		return false;
	}

	hud.image_loc_model = glGetUniformLocation(hud.image_prog, "model");
	hud.image_loc_projection = glGetUniformLocation(hud.image_prog, "projection");

	if (! shader_program_create(RESSOURCEPATH "shaders/hud/font", &hud.font_prog)) {
		fprintf(stderr, "Failed to create HUD font shader program\n");
		return false;
	}

	hud.font_loc_model = glGetUniformLocation(hud.font_prog, "model");
	hud.font_loc_projection = glGetUniformLocation(hud.font_prog, "projection");
	hud.font_loc_text_color = glGetUniformLocation(hud.font_prog, "textColor");

	hud.elements = list_create(NULL);

	hud.image_mesh = mesh_create();
	hud.image_mesh->vertices = image_vertices;
	hud.image_mesh->vertices_count = 6;
	hud.image_mesh->layout = &image_vertex_layout;

	return true;
}

static void free_element(void *key, __attribute__((unused)) void *value, __attribute__((unused)) void *arg)
{
	HUDElement *element = key;

	if (element->def.type == HUD_TEXT) {
		font_delete(element->type_data.text);
		free(element->def.type_def.text.text);
	}

	free(element);
}

void hud_deinit()
{
	glDeleteProgram(hud.image_prog);
	glDeleteProgram(hud.font_prog);
	mesh_delete(hud.image_mesh);
	list_clear_func(&hud.elements, &free_element, NULL);
}

static void element_transform(HUDElement *element)
{
	v3f32 pos = {
		(f32) element->def.offset.x + (1.0f + element->def.pos.x) / 2.0f * (f32) hud.width,
		(f32) element->def.offset.y + (1.0f + element->def.pos.y) / 2.0f * (f32) hud.height,
		element->def.pos.z,
	};

	mat4x4_translate(element->transform, pos.x, pos.y, pos.z);

	if (element->def.type == HUD_IMAGE) {
		v2f32 scale = element->def.type_def.image.scale;

		switch (element->def.type_def.image.scale_type) {
			case HUD_SCALE_TEXTURE:
				scale.x *= element->def.type_def.image.texture->width;
				scale.y *= element->def.type_def.image.texture->height;

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
	}
#pragma GCC diagnostic pop
}

void hud_on_resize(int width, int height)
{
	hud.width = width;
	hud.height = height;

	mat4x4_ortho(hud.projection, 0, width, height, 0, -1.0f, 1.0f);
	glProgramUniformMatrix4fv(hud.image_prog, hud.image_loc_projection, 1, GL_FALSE, hud.projection[0]);
	glProgramUniformMatrix4fv(hud.font_prog, hud.font_loc_projection, 1, GL_FALSE, hud.projection[0]);

	for (ListPair *pair = hud.elements.first; pair != NULL; pair = pair->next)
		element_transform(pair->key);
}

void hud_render()
{
	glActiveTexture(GL_TEXTURE0);

	for (ListPair *pair = hud.elements.first; pair != NULL; pair = pair->next) {
		HUDElement *element = pair->key;

		if (element->visible) {
			switch (element->def.type) {
				case HUD_IMAGE:
					glUseProgram(hud.image_prog);
					glUniformMatrix4fv(hud.image_loc_model, 1, GL_FALSE, element->transform[0]);
					hud.image_mesh->texture = element->def.type_def.image.texture->id;
					mesh_render(hud.image_mesh);

					break;

				case HUD_TEXT:
					glUseProgram(hud.font_prog);
					glUniformMatrix4fv(hud.font_loc_model, 1, GL_FALSE, element->transform[0]);
					glUniform3f(hud.font_loc_text_color, element->def.type_def.text.color.x, element->def.type_def.text.color.y, element->def.type_def.text.color.z);
					font_render(element->type_data.text);

					break;
			};
		}
	}
}

HUDElement *hud_add(HUDElementDefinition def)
{
	HUDElement *element = malloc(sizeof(HUDElement));
	element->def = def;
	element->visible = true;

	element_transform(element);

	if (element->def.type == HUD_TEXT) {
		element->def.type_def.text.text = strdup(element->def.type_def.text.text);
		element->type_data.text = font_create(element->def.type_def.text.text);
	}

	list_set(&hud.elements, element, NULL);

	return element;
}

void hud_change_text(HUDElement *element, char *text)
{
	if (strcmp(element->def.type_def.text.text, text)) {
		element->def.type_def.text.text = strdup(text);
		font_delete(element->type_data.text);
		element->type_data.text = font_create(text);
	}
}
