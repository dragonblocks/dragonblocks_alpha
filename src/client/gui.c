#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include "client/client.h"
#include "client/cube.h"
#include "client/gui.h"
#include "client/mesh.h"
#include "client/shader.h"
#include "client/vertex.h"
#include "util.h"

static struct
{
	List elements;

	GLuint background_prog;
	GLint background_loc_model;
	GLint background_loc_projection;
	GLint background_loc_color;
	Mesh *background_mesh;

	GLuint image_prog;
	GLint image_loc_model;
	GLint image_loc_projection;
	Mesh *image_mesh;

	GLuint font_prog;
	GLint font_loc_model;
	GLint font_loc_projection;
	GLint font_loc_color;

	mat4x4 projection;
} gui;

GUIElement gui_root;

typedef struct
{
	GLfloat x, y;
} __attribute__((packed)) VertexBackgroundPosition;

typedef struct
{
	VertexBackgroundPosition position;
} __attribute__((packed)) VertexBackground;

static VertexAttribute background_vertex_attributes[1] = {
	// position
	{
		.type = GL_FLOAT,
		.length = 2,
		.size = sizeof(VertexBackgroundPosition),
	},
};

static VertexLayout background_vertex_layout = {
	.attributes = background_vertex_attributes,
	.count = 1,
	.size = sizeof(VertexBackground),
};

static VertexBackground background_vertices[6] = {
	{{0.0, 0.0}},
	{{1.0, 0.0}},
	{{1.0, 1.0}},
	{{1.0, 1.0}},
	{{0.0, 1.0}},
	{{0.0, 0.0}},
};

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
	{{0.0, 0.0}, {0.0, 0.0}},
	{{1.0, 0.0}, {1.0, 0.0}},
	{{1.0, 1.0}, {1.0, 1.0}},
	{{1.0, 1.0}, {1.0, 1.0}},
	{{0.0, 1.0}, {0.0, 1.0}},
	{{0.0, 0.0}, {0.0, 0.0}},
};

static int bintree_compare_f32(void *v1, void *v2, unused Bintree *tree)
{
	f32 diff = (*(f32 *) v1) - (*(f32 *) v2);
	return CMPBOUNDS(diff);
}

bool gui_init()
{
	// initialize background pipeline

	if (! shader_program_create(RESSOURCE_PATH "shaders/gui/background", &gui.background_prog, NULL)) {
		fprintf(stderr, "Failed to create GUI background shader program\n");
		return false;
	}

	gui.background_loc_model = glGetUniformLocation(gui.background_prog, "model");
	gui.background_loc_projection = glGetUniformLocation(gui.background_prog, "projection");
	gui.background_loc_color = glGetUniformLocation(gui.background_prog, "color");

	gui.background_mesh = mesh_create();
	gui.background_mesh->textures = NULL;
	gui.background_mesh->textures_count = 0;
	gui.background_mesh->free_textures = false;
	gui.background_mesh->vertices = background_vertices;
	gui.background_mesh->vertices_count = 6;
	gui.background_mesh->free_vertices = false;
	gui.background_mesh->layout = &background_vertex_layout;

	// initialize image pipeline

	if (! shader_program_create(RESSOURCE_PATH "shaders/gui/image", &gui.image_prog, NULL)) {
		fprintf(stderr, "Failed to create GUI image shader program\n");
		return false;
	}

	gui.image_loc_model = glGetUniformLocation(gui.image_prog, "model");
	gui.image_loc_projection = glGetUniformLocation(gui.image_prog, "projection");

	gui.image_mesh = mesh_create();
	gui.image_mesh->textures = NULL;
	gui.image_mesh->textures_count = 1;
	gui.image_mesh->free_textures = false;
	gui.image_mesh->vertices = image_vertices;
	gui.image_mesh->vertices_count = 6;
	gui.image_mesh->free_vertices = false;
	gui.image_mesh->layout = &image_vertex_layout;

	// initialize font pipeline

	if (! shader_program_create(RESSOURCE_PATH "shaders/gui/font", &gui.font_prog, NULL)) {
		fprintf(stderr, "Failed to create GUI font shader program\n");
		return false;
	}

	gui.font_loc_model = glGetUniformLocation(gui.font_prog, "model");
	gui.font_loc_projection = glGetUniformLocation(gui.font_prog, "projection");
	gui.font_loc_color = glGetUniformLocation(gui.font_prog, "color");

	// font meshes are initialized in font.c

	// initialize GUI root element

	gui_root.def.pos = (v2f32) {0.0f, 0.0f};
	gui_root.def.z_index = 0.0f;
	gui_root.def.offset = (v2s32) {0, 0};
	gui_root.def.align = (v2f32) {0.0f, 0.0f};
	gui_root.def.scale = (v2f32) {0.0f, 0.0f};
	gui_root.def.scale_type = GST_NONE;
	gui_root.def.affect_parent_scale = false;
	gui_root.def.text = NULL;
	gui_root.def.image = NULL;
	gui_root.def.text_color = (v4f32) {0.0f, 0.0f, 0.0f, 0.0f};
	gui_root.def.bg_color = (v4f32) {0.0f, 0.0f, 0.0f, 0.0f};
	gui_root.visible = true;
	gui_root.pos = (v2f32)  {0.0f, 0.0f};
	gui_root.scale = (v2f32) {0.0f, 0.0f};
	gui_root.text = NULL;
	gui_root.parent = &gui_root;
	gui_root.children = bintree_create(sizeof(f32), &bintree_compare_f32);

	return true;
}

static void free_element(BintreeNode *node, unused void *arg)
{
	GUIElement *element = node->value;

	bintree_clear(&element->children, &free_element, NULL);

	if (element->def.text)
		free(element->def.text);

	if (element->text)
		font_delete(element->text);

	free(element);
}

void gui_deinit()
{
	glDeleteProgram(gui.background_prog);
	mesh_delete(gui.background_mesh);

	glDeleteProgram(gui.image_prog);
	mesh_delete(gui.image_mesh);

	glDeleteProgram(gui.font_prog);

	bintree_clear(&gui_root.children, &free_element, NULL);
}

void gui_on_resize(int width, int height)
{
	mat4x4_ortho(gui.projection, 0, width, height, 0, -1.0f, 1.0f);
	glProgramUniformMatrix4fv(gui.background_prog, gui.background_loc_projection, 1, GL_FALSE, gui.projection[0]);
	glProgramUniformMatrix4fv(gui.image_prog, gui.image_loc_projection, 1, GL_FALSE, gui.projection[0]);
	glProgramUniformMatrix4fv(gui.font_prog, gui.font_loc_projection, 1, GL_FALSE, gui.projection[0]);

	gui_root.def.scale.x = width;
	gui_root.def.scale.y = height;

	gui_update_transform(&gui_root);
}

static void render_element(BintreeNode *node, unused void *arg)
{
	GUIElement *element = node->value;

	if (element->visible) {
		if (element->def.bg_color.w > 0.0f) {
			glUseProgram(gui.background_prog);
			glUniformMatrix4fv(gui.background_loc_model, 1, GL_FALSE, element->transform[0]);
			glUniform4f(gui.background_loc_color, element->def.bg_color.x, element->def.bg_color.y, element->def.bg_color.z, element->def.bg_color.w);
			mesh_render(gui.background_mesh);
		}

		if (element->def.image) {
			glUseProgram(gui.image_prog);
			glUniformMatrix4fv(gui.image_loc_model, 1, GL_FALSE, element->transform[0]);
			gui.image_mesh->textures = &element->def.image->id;
			mesh_render(gui.image_mesh);
		}

		if (element->text && element->def.text_color.w > 0.0f) {
			glUseProgram(gui.font_prog);
			glUniformMatrix4fv(gui.font_loc_model, 1, GL_FALSE, element->text_transform[0]);
			glUniform4f(gui.font_loc_color, element->def.text_color.x, element->def.text_color.y, element->def.text_color.z, element->def.text_color.w);
			font_render(element->text);
		}

		bintree_traverse(&element->children, BTT_INORDER, &render_element, NULL);
	}
}

void gui_render()
{
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	bintree_traverse(&gui_root.children, BTT_INORDER, &render_element, NULL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}

GUIElement *gui_add(GUIElement *parent, GUIElementDefinition def)
{
	GUIElement *element = malloc(sizeof(GUIElement));
	element->def = def;
	element->visible = true;
	element->parent = parent;

	if (element->def.text) {
		element->def.text = strdup(element->def.text);
		element->text = font_create(element->def.text);
	} else {
		element->text = NULL;
	}

	bintree_insert(&parent->children, &element->def.z_index, element);

	element->children = bintree_create(sizeof(f32), &bintree_compare_f32);

	if (element->def.affect_parent_scale)
		gui_update_transform(parent);
	else
		gui_update_transform(element);

	return element;
}

void gui_set_text(GUIElement *element, char *text)
{
	if (element->def.text)
		free(element->def.text);

	element->def.text = text;
	font_delete(element->text);
	element->text = font_create(text);
	gui_update_transform(element);
}

// transform code

typedef struct
{
	List left_nodes;
	v2f32 result;
} PrecalculateChildrenScaleData;

static void precalculate_children_scale(BintreeNode *node, void *arg);
static void bintree_calculate_element_scale(BintreeNode *node, void *arg);
static void list_calculate_element_scale(void *key, void *value, void *arg);
static void bintree_calculate_element_transform(BintreeNode *node, unused void *arg);

static void calculate_element_scale(GUIElement *element)
{
	element->scale = (v2f32) {
		element->def.scale.x,
		element->def.scale.y,
	};

	bool traversed_children = false;

	switch (element->def.scale_type) {
		case GST_IMAGE:
			element->scale.x *= element->def.image->width;
			element->scale.y *= element->def.image->height;
			break;

		case GST_TEXT:
			element->scale.x *= element->text->size.x;
			element->scale.y *= element->text->size.y;
			break;

		case GST_PARENT:
			element->scale.x *= element->parent->scale.x;
			element->scale.y *= element->parent->scale.y;
			break;

		case GST_CHILDREN: {
			PrecalculateChildrenScaleData pdata = {
				.left_nodes = list_create(NULL),
				.result = {0.0f, 0.0f},
			};

			bintree_traverse(&element->children, BTT_INORDER, &precalculate_children_scale, &pdata);

			element->scale.x *= pdata.result.x;
			element->scale.y *= pdata.result.y;

			list_clear_func(&pdata.left_nodes, &list_calculate_element_scale, NULL);
			traversed_children = true;
		} break;

		case GST_NONE:
			break;
	}

	if (! traversed_children)
		bintree_traverse(&element->children, BTT_INORDER, &bintree_calculate_element_scale, NULL);
}

static void precalculate_children_scale(BintreeNode *node, void *arg)
{
	GUIElement *element = node->value;
	PrecalculateChildrenScaleData *pdata = arg;

	if (element->def.affect_parent_scale) {
		assert(element->def.scale_type != GST_PARENT);
		calculate_element_scale(element);

		if (element->scale.x > pdata->result.x)
			pdata->result.x = element->scale.x;

		if (element->scale.y > pdata->result.y)
			pdata->result.y = element->scale.y;
	} else {
		list_put(&pdata->left_nodes, element, NULL);
	}
}

static void bintree_calculate_element_scale(BintreeNode *node, unused void *arg)
{
	calculate_element_scale(node->value);
}

static void list_calculate_element_scale(void *key, unused void *value, unused void *arg)
{
	calculate_element_scale(key);
}

static void calculate_element_transform(GUIElement *element)
{
	element->pos = (v2f32) {
		floor(element->parent->pos.x + element->def.offset.x + element->def.pos.x * element->parent->scale.x - element->def.align.x * element->scale.x),
		floor(element->parent->pos.y + element->def.offset.y + element->def.pos.y * element->parent->scale.y - element->def.align.y * element->scale.y),
	};

	mat4x4_translate(element->transform, element->pos.x - element->def.margin.x, element->pos.y - element->def.margin.y, 0.0f);
	mat4x4_translate(element->text_transform, element->pos.x, element->pos.y, 0.0f);
	mat4x4_scale_aniso(element->transform, element->transform, element->scale.x + element->def.margin.x * 2.0f, element->scale.y + element->def.margin.y * 2.0f, 1.0f);

	bintree_traverse(&element->children, BTT_INORDER, &bintree_calculate_element_transform, NULL);
}

static void bintree_calculate_element_transform(BintreeNode *node, unused void *arg)
{
	calculate_element_transform(node->value);
}

void gui_update_transform(GUIElement *element)
{
	calculate_element_scale(element);
	calculate_element_transform(element);
}
