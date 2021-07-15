#ifndef _HUD_H_
#define _HUD_H_

#include <stdbool.h>
#include <linmath.h/linmath.h>
#include "client/font.h"
#include "client/texture.h"
#include "types.h"

typedef enum
{
	HUD_SCALE_TEXTURE,
	HUD_SCALE_SCREEN,
	HUD_SCALE_NONE,
} HUDImageScaleType;

typedef enum
{
	HUD_IMAGE,
	HUD_TEXT,
} HUDElementType;

typedef struct
{
	HUDElementType type;
	v3f pos;
	v2s32 offset;
	union
	{
		struct {
			Texture *texture;
			v2f scale;
			HUDImageScaleType scale_type;
		} image;
		struct {
			char *text;
			v3f color;
		} text;
	} type_def;
} HUDElementDefinition;

typedef struct
{
	HUDElementDefinition def;
	bool visible;
	mat4x4 transform;
	union
	{
		Font *text;
	} type_data;
} HUDElement;

bool hud_init();
void hud_deinit();
void hud_on_resize(int width, int height);
void hud_render();
HUDElement *hud_add(HUDElementDefinition def);

#endif
