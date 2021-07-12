#ifndef _HUD_H_
#define _HUD_H_

#include <stdbool.h>
#include <linmath.h/linmath.h>
#include "shaders.h"
#include "texture.h"
#include "types.h"

typedef enum
{
	HUD_SCALE_TEXTURE,
	HUD_SCALE_SCREEN,
	HUD_SCALE_NONE,
} HUDScaleType;

typedef struct
{
	Texture *texture;
	bool visible;
	v3f pos;
	v2f scale;
	HUDScaleType scale_type;
	mat4x4 transform;
} HUDElement;

void hud_init(ShaderProgram *prog);
void hud_deinit();
void hud_rescale(int width, int height);
void hud_render();
HUDElement *hud_add(char *texture, v3f pos, v2f scale, HUDScaleType scale_type);

#endif
