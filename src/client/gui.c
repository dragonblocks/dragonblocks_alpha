#include <GL/glew.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client/client.h"
#include "client/cube.h"
#include "client/gui.h"
#include "client/mesh.h"
#include "client/shader.h"
#include "client/window.h"

static GUIElement root_element;

static GLuint background_prog;
static GLint background_loc_model;
static GLint background_loc_projection;
static GLint background_loc_color;
typedef struct {
	v2f32 position;
} __attribute__((packed)) BackgroundVertex;
static Mesh background_mesh = {
	.layout = &(VertexLayout) {
		.attributes = (VertexAttribute[]) {
			{GL_FLOAT, 2, sizeof(v2f32)}, // position
		},
		.count = 1,
		.size = sizeof(BackgroundVertex),
	},
	.vao = 0,
	.vbo = 0,
	.data = (BackgroundVertex[]) {
		{{0.0, 0.0}},
		{{1.0, 0.0}},
		{{1.0, 1.0}},
		{{1.0, 1.0}},
		{{0.0, 1.0}},
		{{0.0, 0.0}},
	},
	.count = 6,
	.free_data = false,
};

static GLuint image_prog;
static GLint image_loc_model;
static GLint image_loc_projection;
typedef struct {
	v2f32 position;
	v2f32 textureCoordinates;
} __attribute__((packed)) ImageVertex;
static Mesh image_mesh = {
	.layout = &(VertexLayout) {
		.attributes = (VertexAttribute[]) {
			{GL_FLOAT, 2, sizeof(v2f32)}, // position
			{GL_FLOAT, 2, sizeof(v2f32)}, // textureCoordinates
		},
		.count = 2,
		.size = sizeof(ImageVertex),
	},
	.vao = 0,
	.vbo = 0,
	.data = (ImageVertex[]) {
		{{0.0, 0.0}, {0.0, 0.0}},
		{{1.0, 0.0}, {1.0, 0.0}},
		{{1.0, 1.0}, {1.0, 1.0}},
		{{1.0, 1.0}, {1.0, 1.0}},
		{{0.0, 1.0}, {0.0, 1.0}},
		{{0.0, 0.0}, {0.0, 0.0}},
	},
	.count = 6,
	.free_data = false,
};

static GLuint font_prog;
static GLint font_loc_model;
static GLint font_loc_projection;
static GLint font_loc_color;
// font meshes are initialized in font.c

static mat4x4 projection;

// element functions

static void delete_element(GUIElement *element);
static void render_element(GUIElement *element);
static void scale_element(GUIElement *element);

static int cmp_element(const GUIElement *ea, const GUIElement *eb)
{
	return f32_cmp(&ea->def.z_index, &eb->def.z_index);
}

static void delete_elements(Array *elements)
{
	for (size_t i = 0; i < elements->siz; i++)
		delete_element(((GUIElement **) elements->ptr)[i]);
	array_clr(elements);
}

static void delete_element(GUIElement *element)
{
	delete_elements(&element->children);

	if (element->def.text)
		free(element->def.text);

	if (element->text)
		font_delete(element->text);

	free(element);
}

static void render_elements(Array *elements)
{
	for (size_t i = 0; i < elements->siz; i++)
		render_element(((GUIElement **) elements->ptr)[i]);
}

static void render_element(GUIElement *element)
{
	if (element->visible) {
		if (element->def.bg_color.w > 0.0f) {
			glUseProgram(background_prog);
			glUniformMatrix4fv(background_loc_model, 1, GL_FALSE, element->transform[0]);
			glUniform4f(background_loc_color, element->def.bg_color.x, element->def.bg_color.y, element->def.bg_color.z, element->def.bg_color.w);
			mesh_render(&background_mesh);
		}

		if (element->def.image) {
			glUseProgram(image_prog);
			glUniformMatrix4fv(image_loc_model, 1, GL_FALSE, element->transform[0]);
			glBindTextureUnit(0, element->def.image->txo);
			mesh_render(&image_mesh);
		}

		if (element->text && element->def.text_color.w > 0.0f) {
			glUseProgram(font_prog);
			glUniformMatrix4fv(font_loc_model, 1, GL_FALSE, element->text_transform[0]);
			glUniform4f(font_loc_color, element->def.text_color.x, element->def.text_color.y, element->def.text_color.z, element->def.text_color.w);
			font_render(element->text);
		}

		render_elements(&element->children);
	}
}

static void scale_elements(Array *elements, int mask, v3f32 *max)
{
	for (size_t i = 0; i < elements->siz; i++) {
		GUIElement *element = ((GUIElement **) elements->ptr)[i];

		if ((1 << element->def.affect_parent_scale) & mask) {
			scale_element(element);

			if (max) {
				if (element->scale.x > max->x)
					max->x = element->scale.x;

				if (element->scale.y > max->y)
					max->y = element->scale.y;
			}
		}
	}
}

static void scale_element(GUIElement *element)
{
	element->scale = (v2f32) {
		element->def.scale.x,
		element->def.scale.y,
	};

	switch (element->def.scale_type) {
		case SCALE_IMAGE:
			element->scale.x *= element->def.image->width;
			element->scale.y *= element->def.image->height;
			break;

		case SCALE_TEXT:
			element->scale.x *= element->text->size.x;
			element->scale.y *= element->text->size.y;
			break;

		case SCALE_PARENT:
			element->scale.x *= element->parent->scale.x;
			element->scale.y *= element->parent->scale.y;
			break;

		case SCALE_CHILDREN: {
			v3f32 scale = {0.0f, 0.0f, 0.0f};
			scale_elements(&element->children, 1 << true, &scale);

			element->scale.x *= scale.x;
			element->scale.y *= scale.y;

			scale_elements(&element->children, 1 << false, NULL);
			break;
		}

		case SCALE_NONE:
			break;
	}

	if (element->def.scale_type != SCALE_CHILDREN)
		scale_elements(&element->children, (1 << true) | (1 << false), NULL);
}

static void transform_element(GUIElement *element)
{
	element->pos = (v2f32) {
		floor(element->parent->pos.x + element->def.offset.x + element->def.pos.x * element->parent->scale.x - element->def.align.x * element->scale.x),
		floor(element->parent->pos.y + element->def.offset.y + element->def.pos.y * element->parent->scale.y - element->def.align.y * element->scale.y),
	};

	mat4x4_translate(element->transform, element->pos.x - element->def.margin.x, element->pos.y - element->def.margin.y, 0.0f);
	mat4x4_translate(element->text_transform, element->pos.x, element->pos.y, 0.0f);
	mat4x4_scale_aniso(element->transform, element->transform, element->scale.x + element->def.margin.x * 2.0f, element->scale.y + element->def.margin.y * 2.0f, 1.0f);

	for (size_t i = 0; i < element->children.siz; i++)
		transform_element(((GUIElement **) element->children.ptr)[i]);
}

// public functions

bool gui_init()
{
	// initialize background pipeline

	if (!shader_program_create(RESSOURCE_PATH "shaders/gui/background", &background_prog, NULL)) {
		fprintf(stderr, "[error] failed to create GUI background shader program\n");
		return false;
	}

	background_loc_model = glGetUniformLocation(background_prog, "model");
	background_loc_projection = glGetUniformLocation(background_prog, "projection");
	background_loc_color = glGetUniformLocation(background_prog, "color");

	// initialize image pipeline

	if (!shader_program_create(RESSOURCE_PATH "shaders/gui/image", &image_prog, NULL)) {
		fprintf(stderr, "[error] failed to create GUI image shader program\n");
		return false;
	}

	image_loc_model = glGetUniformLocation(image_prog, "model");
	image_loc_projection = glGetUniformLocation(image_prog, "projection");

	// initialize font pipeline

	if (!shader_program_create(RESSOURCE_PATH "shaders/gui/font", &font_prog, NULL)) {
		fprintf(stderr, "[error] failed to create GUI font shader program\n");
		return false;
	}

	font_loc_model = glGetUniformLocation(font_prog, "model");
	font_loc_projection = glGetUniformLocation(font_prog, "projection");
	font_loc_color = glGetUniformLocation(font_prog, "color");

	// initialize GUI root element

	root_element.def.pos = (v2f32) {0.0f, 0.0f};
	root_element.def.z_index = 0.0f;
	root_element.def.offset = (v2s32) {0, 0};
	root_element.def.align = (v2f32) {0.0f, 0.0f};
	root_element.def.scale = (v2f32) {0.0f, 0.0f};
	root_element.def.scale_type = SCALE_NONE;
	root_element.def.affect_parent_scale = false;
	root_element.def.text = NULL;
	root_element.def.image = NULL;
	root_element.def.text_color = (v4f32) {0.0f, 0.0f, 0.0f, 0.0f};
	root_element.def.bg_color = (v4f32) {0.0f, 0.0f, 0.0f, 0.0f};
	root_element.visible = true;
	root_element.pos = (v2f32)  {0.0f, 0.0f};
	root_element.scale = (v2f32) {0.0f, 0.0f};
	root_element.text = NULL;
	root_element.parent = &root_element;
	array_ini(&root_element.children, sizeof(GUIElement *), 0);

	gui_update_projection();

	return true;
}

void gui_deinit()
{
	glDeleteProgram(background_prog);
	mesh_destroy(&background_mesh);

	glDeleteProgram(image_prog);
	mesh_destroy(&image_mesh);

	glDeleteProgram(font_prog);

	delete_elements(&root_element.children);
}

void gui_update_projection()
{
	mat4x4_ortho(projection, 0, window.width, window.height, 0, -1.0f, 1.0f);
	glProgramUniformMatrix4fv(background_prog, background_loc_projection, 1, GL_FALSE, projection[0]);
	glProgramUniformMatrix4fv(image_prog, image_loc_projection, 1, GL_FALSE, projection[0]);
	glProgramUniformMatrix4fv(font_prog, font_loc_projection, 1, GL_FALSE, projection[0]);

	root_element.def.scale.x = window.width;
	root_element.def.scale.y = window.height;

	gui_transform(&root_element);
}

void gui_render()
{
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);

	render_elements(&root_element.children);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}

GUIElement *gui_add(GUIElement *parent, GUIElementDefinition def)
{
	if (parent == NULL)
		parent = &root_element;

	GUIElement *element = malloc(sizeof *element);
	element->def = def;
	element->visible = true;
	element->parent = parent;

	if (element->def.text) {
		element->def.text = strdup(element->def.text);
		element->text = font_create(element->def.text);
	} else {
		element->text = NULL;
	}

	array_ins(&parent->children, &element, (void *) &cmp_element);
	array_ini(&element->children, sizeof(GUIElement), 0);

	if (element->def.affect_parent_scale)
		gui_transform(parent);
	else
		gui_transform(element);

	return element;
}

void gui_text(GUIElement *element, char *text)
{
	if (element->def.text)
		free(element->def.text);

	element->def.text = text;
	font_delete(element->text);
	element->text = font_create(text);
	gui_transform(element);
}

void gui_transform(GUIElement *element)
{
	scale_element(element);
	transform_element(element);
}
