#include <stdlib.h>
#include <pthread.h>
#include "color.h"
#include "perlin.h"
#include "server/terrain_gen.h"
#include "server/voxelctx.h"

static VoxelctxState *create_state(VoxelctxState *old)
{
	VoxelctxState *state = malloc(sizeof *state);

	if (old) {
		*state = *old;
	} else {
		state->pos[0] = 0.0f;
		state->pos[1] = 0.0f;
		state->pos[2] = 0.0f;
		state->pos[3] = 0.0f;
		state->scale[0] = 1.0f;
		state->scale[1] = 1.0f;
		state->scale[2] = 1.0f;
		mat4x4_identity(state->mat);
		state->h = 0.0f;
		state->s = 0.0f;
		state->l = 1.0f;
		state->life = 0;
	}

	return state;
}

Voxelctx *voxelctx_create(List *changed_chunks, TerrainGenStage tgs, v3s32 pos)
{
	Voxelctx *ctx = malloc(sizeof(Voxelctx));

	ctx->changed_chunks = changed_chunks;
	ctx->tgs = tgs;
	ctx->pos = pos;
	list_ini(&ctx->statestack);
	ctx->random = 0;

	list_apd(&ctx->statestack, create_state(NULL));

	return ctx;
}

void voxelctx_delete(Voxelctx *ctx)
{
	list_clr(&ctx->statestack, (void *) &free, NULL, NULL);
	free(ctx);
}

static void move_value(f32 *x, f32 v, f32 range)
{
    f32 dst = v >= 0 ? range : 0;
    v = fabs(v);
    *x = f32_mix(*x, dst, v);
}

void voxelctx_hue(Voxelctx *ctx, f32 value)
{
	VOXELCTXSTATE(ctx).h += value;
}

void voxelctx_sat(Voxelctx *ctx, f32 value)
{
	move_value(&VOXELCTXSTATE(ctx).s, value, 1.0f);
}

void voxelctx_light(Voxelctx *ctx, f32 value)
{
	move_value(&VOXELCTXSTATE(ctx).l, value, 1.0f);
}

void voxelctx_life(Voxelctx *ctx, s32 value)
{
	VOXELCTXSTATE(ctx).life += value;
}

static void apply_translation(Voxelctx *ctx, v3f32 translate)
{
	vec4 translate_vec;
	mat4x4_mul_vec4(translate_vec, VOXELCTXSTATE(ctx).mat, (vec4) {translate.x, translate.y, translate.z, 0.0f});
	vec4_add(VOXELCTXSTATE(ctx).pos, VOXELCTXSTATE(ctx).pos, translate_vec);
}

void voxelctx_x(Voxelctx *ctx, f32 value)
{
	apply_translation(ctx, (v3f32) {value, 0.0f, 0.0f});
}

void voxelctx_y(Voxelctx *ctx, f32 value)
{
	apply_translation(ctx, (v3f32) {0.0f, value, 0.0f});
}

void voxelctx_z(Voxelctx *ctx, f32 value)
{
	apply_translation(ctx, (v3f32) {0.0f, 0.0f, value});
}

void voxelctx_rx(Voxelctx *ctx, f32 value)
{
	mat4x4_rotate_X(VOXELCTXSTATE(ctx).mat, VOXELCTXSTATE(ctx).mat, value * M_PI / 180.0f);
}

void voxelctx_ry(Voxelctx *ctx, f32 value)
{
	mat4x4_rotate_Y(VOXELCTXSTATE(ctx).mat, VOXELCTXSTATE(ctx).mat, value * M_PI / 180.0f);
}

void voxelctx_rz(Voxelctx *ctx, f32 value)
{
	mat4x4_rotate_Z(VOXELCTXSTATE(ctx).mat, VOXELCTXSTATE(ctx).mat, value * M_PI / 180.0f);
}

static void apply_scale(Voxelctx *ctx, v3f32 scale)
{
	VOXELCTXSTATE(ctx).scale[0] *= scale.x;
	VOXELCTXSTATE(ctx).scale[1] *= scale.y;
	VOXELCTXSTATE(ctx).scale[2] *= scale.z;

	mat4x4_scale_aniso(VOXELCTXSTATE(ctx).mat, VOXELCTXSTATE(ctx).mat, scale.x, scale.y, scale.z);
}

void voxelctx_sx(Voxelctx *ctx, f32 value)
{
	apply_scale(ctx, (v3f32) {value, 1.0f, 1.0f});
}

void voxelctx_sy(Voxelctx *ctx, f32 value)
{
	apply_scale(ctx, (v3f32) {1.0f, value, 1.0f});
}

void voxelctx_sz(Voxelctx *ctx, f32 value)
{
	apply_scale(ctx, (v3f32) {1.0f, 1.0f, value});
}

void voxelctx_s(Voxelctx *ctx, f32 value)
{
	apply_scale(ctx, (v3f32) {value, value, value});
}

void voxelctx_pop(Voxelctx *ctx)
{
	free(ctx->statestack.fst->dat);
	list_nrm(&ctx->statestack, &ctx->statestack.fst);
}

void voxelctx_push(Voxelctx *ctx)
{
	list_ppd(&ctx->statestack, create_state(&VOXELCTXSTATE(ctx)));
}

bool voxelctx_is_alive(Voxelctx *ctx)
{
	if (VOXELCTXSTATE(ctx).life > 0) {
		VOXELCTXSTATE(ctx).life--;
		if (VOXELCTXSTATE(ctx).life <= 0)
			return false;
	}

	return VOXELCTXSTATE(ctx).scale[0] >= 1.0f && VOXELCTXSTATE(ctx).scale[1] >= 1.0f && VOXELCTXSTATE(ctx).scale[2] >= 1.0f;
}

void voxelctx_cube(Voxelctx *ctx, NodeType node, bool use_color)
{
	if (!voxelctx_is_alive(ctx))
		return;

	vec4 base_corners[8] = {
		{0.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 1.0f, 0.0f},
		{0.0f, 1.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 1.0f, 0.0f},
		{1.0f, 0.0f, 0.0f, 0.0f},
		{1.0f, 0.0f, 1.0f, 0.0f},
		{1.0f, 1.0f, 0.0f, 0.0f},
		{1.0f, 1.0f, 1.0f, 0.0f},
	};

	vec4 corners[8];

	s32 max_len = 0;

	vec4 center;

	mat4x4_mul_vec4(center, VOXELCTXSTATE(ctx).mat, (vec4) {0.5f, 0.5f, 0.5f});

	for (int i = 0; i < 8; i++) {
		mat4x4_mul_vec4(corners[i], VOXELCTXSTATE(ctx).mat, base_corners[i]);

		vec3 from_center;
		vec3_sub(from_center, corners[i], center);

		s32 len = ceil(vec3_len(from_center));

		if (max_len < len)
			max_len = len;
	}

	for (s32 x = -max_len; x <= +max_len; x++)
	for (s32 y = -max_len; y <= +max_len; y++)
	for (s32 z = -max_len; z <= +max_len; z++) {
		s32 v[3];

		for (int i = 0; i < 3; i++) {
			f32 f = trunc(
				+ f32_mix(corners[0][i], corners[4][i], (f32) x / (f32) max_len / 2.0f)
				+ f32_mix(corners[0][i], corners[2][i], (f32) y / (f32) max_len / 2.0f)
				+ f32_mix(corners[0][i], corners[1][i], (f32) z / (f32) max_len / 2.0f));

			v[i] = floor(VOXELCTXSTATE(ctx).pos[i] + f + 0.5f);
		}

		Blob buffer = {0, NULL};

		if (use_color)
			ColorData_write(&buffer, &(ColorData) {hsl_to_rgb((v3f32) {
				VOXELCTXSTATE(ctx).h / 360.0,
				VOXELCTXSTATE(ctx).s,
				VOXELCTXSTATE(ctx).l,
			})});

		server_terrain_gen_node(
			v3s32_add(ctx->pos, (v3s32) {v[0], v[2], v[1]}),
			terrain_node_create(node, buffer),
			ctx->tgs,
			ctx->changed_chunks
		);

		Blob_free(&buffer);
	}
}


void voxelctx_cylinder(Voxelctx *ctx, NodeType node, bool use_color)
{
	voxelctx_cube(ctx, node, use_color);
}

/*
void voxelctx_cylinder(Voxelctx *ctx, Node node, bool use_color)
{
	if (!voxelctx_is_alive(ctx))
		return;

	return;

	f32 xf = VOXELCTXSTATE(ctx).scale[0] / 2.0f;
	for (s32 x = round(-xf + 0.5f); x <= round(xf); x++) {
		f32 yf = cos(x / VOXELCTXSTATE(ctx).scale[0] * M_PI) * VOXELCTXSTATE(ctx).scale[1] / 2.0f;
		for (s32 y = round(-yf); y <= round(yf); y++) {
			f32 zf = VOXELCTXSTATE(ctx).scale[2] / 2.0f;
			for (s32 z = round(-zf + 0.5f); z <= round(zf); z++) {
				mapgen_set_node((v3s32) {
					ctx->pos.x + round(VOXELCTXSTATE(ctx).pos[0] + x),
					ctx->pos.y + round(VOXELCTXSTATE(ctx).pos[2] + z),
					ctx->pos.z + round(VOXELCTXSTATE(ctx).pos[1] + y),
				}, CREATE_NODE, ctx->tgs, ctx->changed_chunks);
			}
		}
	}
}
*/

f32 voxelctx_random(Voxelctx *ctx, f32 base, f32 vary)
{
	return base + noise3d(ctx->pos.x, ctx->pos.y, ctx->pos.z, ctx->random++, seed + SO_VOXELCTX) * vary;
}
