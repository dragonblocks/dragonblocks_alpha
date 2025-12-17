#include <GL/glew.h>
#include <GL/gl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client/client.h"
#include "client/cube.h"
#include "client/opengl.h"
#include "client/gui.h"
#include "client/mesh.h"
#include "client/shader.h"
#include "client/window.h"

static GUIElement root_element = {
	.def = {
		.pos = {0.0f, 0.0f},
		.z_index = 0.0f,
		.offset = {0, 0},
		.align = {0.0f, 0.0f},
		.scale = {0.0f, 0.0f},
		.scale_type = SCALE_NONE,
		.affect_parent_scale = false,
		.text = NULL,
		.image = NULL,
		.text_color = {0.0f, 0.0f, 0.0f, 0.0f},
		.bg_color = {0.0f, 0.0f, 0.0f, 0.0f},
	},
	.visible = true,
	.pos = (v2f32)  {0.0f, 0.0f},
	.scale = (v2f32) {0.0f, 0.0f},
	.text = NULL,
	.parent = &root_element,
};

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

static GLuint mesh_prog;
static GLint mesh_loc_model;
static GLint mesh_loc_projection;
// meshes are initialized in mesh.c

static mat4x4 projection;

// element functions

static void delete_element(GUIElement *element);
static void render_element(GUIElement *element);
static void scale_element(GUIElement *element);

static int cmp_element(const GUIElement **ea, const GUIElement **eb)
{
	return f32_cmp(&(*ea)->def.z_index, &(*eb)->def.z_index);
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

	free(element->user);
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
	if (!element->visible)
		return;

	if (element->def.bg_color.w > 0.0f) {
		glUseProgram(background_prog); GL_DEBUG
		glUniformMatrix4fv(background_loc_model, 1, GL_FALSE, element->transform[0]); GL_DEBUG
		glUniform4f(background_loc_color, element->def.bg_color.x, element->def.bg_color.y, element->def.bg_color.z, element->def.bg_color.w); GL_DEBUG
		mesh_render(&background_mesh);
	}

	if (element->def.image) {
		glUseProgram(image_prog); GL_DEBUG
		glUniformMatrix4fv(image_loc_model, 1, GL_FALSE, element->transform[0]); GL_DEBUG
		bind_texture_2d(0, element->def.image->txo); GL_DEBUG
		mesh_render(&image_mesh);
	}

	if (element->def.text && element->def.text_color.w > 0.0f) {
		if (!element->text) {
			element->text = font_create(element->def.text);
			gui_transform(element);
		}

		glUseProgram(font_prog); GL_DEBUG
		glUniformMatrix4fv(font_loc_model, 1, GL_FALSE, element->text_transform[0]); GL_DEBUG
		glUniform4f(font_loc_color, element->def.text_color.x, element->def.text_color.y, element->def.text_color.z, element->def.text_color.w); GL_DEBUG
		font_render(element->text);
	}

	if (element->def.mesh) {
		glEnable(GL_DEPTH_TEST); GL_DEBUG

		glUseProgram(mesh_prog); GL_DEBUG
		glUniformMatrix4fv(mesh_loc_model, 1, GL_FALSE, element->mesh_transform[0]); GL_DEBUG
		mesh_render(element->def.mesh);

		glDisable(GL_DEPTH_TEST); GL_DEBUG
	}

	render_elements(&element->children);
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
			if (!element->text)
				break;

			element->scale.x *= element->text->size.x;
			element->scale.y *= element->text->size.y;
			break;

		case SCALE_PARENT:
			element->scale.x *= element->parent->scale.x;
			element->scale.y *= element->parent->scale.y;
			break;

		case SCALE_RATIO: {
			element->scale.x *= element->parent->scale.x;
			element->scale.y *= element->parent->scale.y;

			if (element->scale.x / element->scale.y > element->def.ratio)
				element->scale.x = element->scale.y * element->def.ratio;
			else
				element->scale.y = element->scale.x / element->def.ratio;
			break;
		}

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
		floor(element->parent->pos.x + 0.5 + element->def.offset.x + element->def.pos.x * element->parent->scale.x - element->def.align.x * element->scale.x),
		floor(element->parent->pos.y + 0.5 + element->def.offset.y + element->def.pos.y * element->parent->scale.y - element->def.align.y * element->scale.y),
	};

	mat4x4_translate(element->transform, element->pos.x - element->def.margin.x, element->pos.y - element->def.margin.y, 0.0f);
	mat4x4_translate(element->text_transform, element->pos.x, element->pos.y, 0.0f);
	mat4x4_scale_aniso(element->transform, element->transform, element->scale.x + element->def.margin.x * 2.0f, element->scale.y + element->def.margin.y * 2.0f, 1.0f);
	mat4x4_mul(element->mesh_transform, element->transform, element->def.mesh_transform);

	for (size_t i = 0; i < element->children.siz; i++)
		transform_element(((GUIElement **) element->children.ptr)[i]);
}

// public functions

void gui_init()
{
	// initialize background pipeline
	background_prog = shader_program_create(ASSET_PATH "shaders/gui/background", NULL);
	background_loc_model = glGetUniformLocation(background_prog, "model"); GL_DEBUG
	background_loc_projection = glGetUniformLocation(background_prog, "projection"); GL_DEBUG
	background_loc_color = glGetUniformLocation(background_prog, "color"); GL_DEBUG

	// initialize image pipeline
	image_prog = shader_program_create(ASSET_PATH "shaders/gui/image", NULL);
	image_loc_model = glGetUniformLocation(image_prog, "model"); GL_DEBUG
	image_loc_projection = glGetUniformLocation(image_prog, "projection"); GL_DEBUG

	// initialize font pipeline
	font_prog = shader_program_create(ASSET_PATH "shaders/gui/font", NULL);
	font_loc_model = glGetUniformLocation(font_prog, "model"); GL_DEBUG
	font_loc_projection = glGetUniformLocation(font_prog, "projection"); GL_DEBUG
	font_loc_color = glGetUniformLocation(font_prog, "color"); GL_DEBUG

	// initialize mesh pipeline
	mesh_prog = shader_program_create(ASSET_PATH "shaders/gui/mesh", NULL);
	mesh_loc_model = glGetUniformLocation(mesh_prog, "model"); GL_DEBUG
	mesh_loc_projection = glGetUniformLocation(mesh_prog, "projection"); GL_DEBUG

	// initialize GUI root element
	array_ini(&root_element.children, sizeof(GUIElement *), 0);
	gui_update_projection();
}

void gui_deinit()
{
	glDeleteProgram(background_prog); GL_DEBUG
	mesh_destroy(&background_mesh);

	glDeleteProgram(image_prog); GL_DEBUG
	mesh_destroy(&image_mesh);

	glDeleteProgram(font_prog); GL_DEBUG

	delete_elements(&root_element.children);
}

void gui_update_projection()
{
	mat4x4_ortho(projection, 0, window.width, window.height, 0, -1.0f, 1.0f);
	glProgramUniformMatrix4fv(background_prog, background_loc_projection, 1, GL_FALSE, projection[0]); GL_DEBUG
	glProgramUniformMatrix4fv(image_prog, image_loc_projection, 1, GL_FALSE, projection[0]); GL_DEBUG
	glProgramUniformMatrix4fv(font_prog, font_loc_projection, 1, GL_FALSE, projection[0]); GL_DEBUG
	glProgramUniformMatrix4fv(mesh_prog, mesh_loc_projection, 1, GL_FALSE, projection[0]); GL_DEBUG

	root_element.def.scale.x = window.width;
	root_element.def.scale.y = window.height;

	gui_transform(&root_element);
}

void gui_render()
{
	glDisable(GL_CULL_FACE); GL_DEBUG
	glDisable(GL_DEPTH_TEST); GL_DEBUG

	render_elements(&root_element.children);

	glEnable(GL_DEPTH_TEST); GL_DEBUG
	glEnable(GL_CULL_FACE); GL_DEBUG
}

GUIElement *gui_add(GUIElement *parent, GUIElementDef def)
{
	if (parent == NULL)
		parent = &root_element;

	GUIElement *element = malloc(sizeof *element);
	element->def = def;
	element->visible = true;
	element->parent = parent;
	element->text = NULL;
	element->user = NULL;


	if (element->def.text)
		element->def.text = strdup(element->def.text);

	array_ins(&parent->children, &element, &cmp_element);
	array_ini(&element->children, sizeof(GUIElement *), 0);

	if (element->def.affect_parent_scale)
		gui_transform(parent);
	else
		gui_transform(element);

	return element;
}

void gui_text(GUIElement *element, const char *text)
{
	if (element->def.text)
		free(element->def.text);

	if (element->text)
		font_delete(element->text);

	element->def.text = strdup(text);
	element->text = NULL;
}

void gui_transform(GUIElement *element)
{
	scale_element(element);
	transform_element(element);
}

static bool click_element(GUIElement *element, float x, float y, bool right)
{
	if (!element->visible)
		return false;

	for (size_t i = 0; i < element->children.siz; i++)
		if (click_element(((GUIElement **) element->children.ptr)[i], x, y, right))
			return true;

	if (!element->def.on_click)
		return false;

	float off_x = x - element->pos.x;
	float off_y = y - element->pos.y;
	if (off_x < 0 || off_y < 0 || off_x > element->scale.x || off_y > element->scale.y)
		return false;

	element->def.on_click(element, right);
	return true;
}

void gui_click(float x, float y, bool right)
{
	click_element(&root_element, x, y, right);
}
