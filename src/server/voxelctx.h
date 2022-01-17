#ifndef _VOXELCTX_H_
#define _VOXELCTX_H_

#define VOXELCTXSTATE(ctx) (*((VoxelctxState *) (ctx)->statestack.first->key))

#include <linmath.h/linmath.h>
#include <dragontype/list.h>
#include <dragontype/number.h>
#include "server/server_map.h"

typedef struct
{
	vec4 pos;
	vec3 scale;
	mat4x4 mat;
	f32 h, s, l;
	s32 life;
} VoxelctxState;

typedef struct
{
	v3s32 pos;
	List *changed_blocks;
	MapgenStage mgs;
	List statestack;
	s32 random;
} Voxelctx;

Voxelctx *voxelctx_create(List *changed_blocks, MapgenStage mgs, v3s32 pos);
void voxelctx_delete(Voxelctx *ctx);
void voxelctx_hue(Voxelctx *ctx, f32 value);
void voxelctx_sat(Voxelctx *ctx, f32 value);
void voxelctx_light(Voxelctx *ctx, f32 value);
void voxelctx_life(Voxelctx *ctx, s32 value);
void voxelctx_x(Voxelctx *ctx, f32 value);
void voxelctx_y(Voxelctx *ctx, f32 value);
void voxelctx_z(Voxelctx *ctx, f32 value);
void voxelctx_rx(Voxelctx *ctx, f32 value);
void voxelctx_ry(Voxelctx *ctx, f32 value);
void voxelctx_rz(Voxelctx *ctx, f32 value);
void voxelctx_sx(Voxelctx *ctx, f32 value);
void voxelctx_sy(Voxelctx *ctx, f32 value);
void voxelctx_sz(Voxelctx *ctx, f32 value);
void voxelctx_s(Voxelctx *ctx, f32 value);
void voxelctx_pop(Voxelctx *ctx);
void voxelctx_push(Voxelctx *ctx);
bool voxelctx_is_alive(Voxelctx *ctx);
void voxelctx_cube(Voxelctx *ctx, Node node, bool use_hsl);
void voxelctx_cylinder(Voxelctx *ctx, Node node, bool use_hsl);
f32 voxelctx_random(Voxelctx *ctx, f32 base, f32 vary);

#endif
