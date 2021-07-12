#ifndef _HUD_H_
#define _HUD_H_

#include <stdbool.h>
#include <linmath.h/linmath.h>
#include "shaders.h"
#include "texture.h"
#include "types.h"

typedef struct
{
	Texture *texture;
	bool visible;
	v2f pos;
	v2f scale;
	mat4x4 transform;
} HUDElement;

void hud_init(ShaderProgram *prog);
void hud_deinit();
void hud_rescale(int width, int height);
void hud_render();
HUDElement *hud_add(char *texture, v2f pos, v2f scale);

#endif
