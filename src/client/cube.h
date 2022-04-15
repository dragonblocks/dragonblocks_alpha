#ifndef _CUBE_H_
#define _CUBE_H_

#include "client/model.h"
#include "types.h"

typedef struct {
	v3f32 position;
	v3f32 normal;
	v2f32 textureCoordinates;
} __attribute__((packed)) CubeVertex;

extern CubeVertex cube_vertices[6][6];

#endif // _CUBE_H_
