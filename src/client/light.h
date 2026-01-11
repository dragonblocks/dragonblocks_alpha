#ifndef _LIGHT_H_
#define _LIGHT_H_

#include "client/opengl.h"

typedef struct {
	GLuint prog;
	GLint loc_daylight;
	GLint loc_fogColor;
	GLint loc_ambientLight;
	GLint loc_lightDir;
	GLint loc_cameraPos;
} LightShader;

void light_shader_locate(LightShader *shader);
void light_shader_update(LightShader *shader);

#endif
