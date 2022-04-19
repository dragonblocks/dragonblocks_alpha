#ifndef _GUI_H_
#define _GUI_H_

#include <dragonstd/array.h>
#include <linmath.h/linmath.h>
#include <stdbool.h>
#include "client/font.h"
#include "client/texture.h"
#include "types.h"

typedef enum {
	SCALE_IMAGE,
	SCALE_TEXT,
	SCALE_PARENT,
	SCALE_CHILDREN,
	SCALE_NONE,
} GUIScaleType;

typedef struct {
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
} GUIElementDef;

typedef struct GUIElement {
	GUIElementDef def;
	bool visible;
	v2f32 pos;
	v2f32 scale;
	mat4x4 transform;
	mat4x4 text_transform;
	Font *text;
	struct GUIElement *parent;
	Array children;
} GUIElement;

bool gui_init();
void gui_deinit();
void gui_update_projection();
void gui_render();
GUIElement *gui_add(GUIElement *parent, GUIElementDef def);
void gui_text(GUIElement *element, const char *text);
void gui_transform(GUIElement *element);

#endif // _GUI_H_
