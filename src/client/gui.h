#ifndef _GUI_H_
#define _GUI_H_

#include <stdbool.h>
#include <linmath.h/linmath.h>
#include <dragonstd/bintree.h>
#include <dragonstd/list.h>
#include "client/font.h"
#include "client/texture.h"
#include "types.h"

typedef enum
{
	GST_IMAGE,
	GST_TEXT,
	GST_PARENT,
	GST_CHILDREN,
	GST_NONE,
} GUIScaleType;

typedef struct
{
	v2f32 pos;
	f32 z_index;
	v2s32 offset;
	v2s32 margin;
	v2f32 align;
	v2f32 scale;
	GUIScaleType scale_type;
	bool affect_parent_scale;
	char *text;
	Texture *image;
	v4f32 text_color;
	v4f32 bg_color;
} GUIElementDefinition;

typedef struct GUIElement
{
	GUIElementDefinition def;
	bool visible;
	v2f32 pos;
	v2f32 scale;
	mat4x4 transform;
	mat4x4 text_transform;
	Font *text;
	struct GUIElement *parent;
	Bintree children;
} GUIElement;

bool gui_init();
void gui_deinit();
void gui_on_resize(int width, int height);
void gui_render();
GUIElement *gui_add(GUIElement *parent, GUIElementDefinition def);
void gui_set_text(GUIElement *element, char *text);
void gui_update_transform(GUIElement *element);

extern GUIElement gui_root;

#endif
