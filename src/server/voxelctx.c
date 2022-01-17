#include <stdlib.h>
#include <pthread.h>
#include "server/mapgen.h"
#include "server/voxelctx.h"
#include "perlin.h"
#include "util.h"

#define CREATE_NODE map_node_create(node, use_color ? (f32[]) {VOXELCTXSTATE(ctx).h / 360.0, VOXELCTXSTATE(ctx).s, VOXELCTXSTATE(ctx).l} : NULL, use_color ? sizeof(f32) * 3 : 0)

static VoxelctxState *create_state(VoxelctxState *old)
{
	VoxelctxState *state = malloc(sizeof(VoxelctxState));

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

Voxelctx *voxelctx_create(List *changed_blocks, MapgenStage mgs, v3s32 pos)
{
	Voxelctx *ctx = malloc(sizeof(Voxelctx));

	ctx->changed_blocks = changed_blocks;
	ctx->mgs = mgs;
	ctx->pos = pos;
	ctx->statestack = list_create(NULL);
	ctx->random = 0;

	list_put(&ctx->statestack, create_state(NULL), NULL);

	return ctx;
}

static void list_delete_state(void *key, unused void *value, unused void *arg)
{
	free(key);
}

void voxelctx_delete(Voxelctx *ctx)
{
	list_clear_func(&ctx->statestack, &list_delete_state, NULL);
	free(ctx);
}

static inline f32 mix(f32 x, f32 y, f32 t)
{
    return (1.0 - t) * x + t * y;
}

static void move_value(f32 *x, f32 v, f32 range)
{
    f32 dst = v >= 0 ? range : 0;
    v = fabs(v);
    *x = mix(*x, dst, v);
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
	ListPair *next = ctx->statestack.first->next;
	free(ctx->statestack.first->key);
	free(ctx->statestack.first);
	ctx->statestack.first = next;
}

void voxelctx_push(Voxelctx *ctx)
{
	ListPair *pair = malloc(sizeof(ListPair));
	pair->key = create_state(&VOXELCTXSTATE(ctx));
	pair->value = NULL;
	pair->next = ctx->statestack.first;

	ctx->statestack.first = pair;
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

void voxelctx_cube(Voxelctx *ctx, Node node, bool use_color)
{
	if (! voxelctx_is_alive(ctx))
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
				+ mix(corners[0][i], corners[4][i], (f32) x / (f32) max_len / 2.0f)
				+ mix(corners[0][i], corners[2][i], (f32) y / (f32) max_len / 2.0f)
				+ mix(corners[0][i], corners[1][i], (f32) z / (f32) max_len / 2.0f));

			v[i] = floor(VOXELCTXSTATE(ctx).pos[i] + f + 0.5f);
		}

		mapgen_set_node(v3s32_add(ctx->pos, (v3s32) {v[0], v[2], v[1]}), CREATE_NODE, ctx->mgs, ctx->changed_blocks);
	}
}


void voxelctx_cylinder(Voxelctx *ctx, Node node, bool use_color)
{
	voxelctx_cube(ctx, node, use_color);
}

/*
void voxelctx_cylinder(Voxelctx *ctx, Node node, bool use_color)
{
	if (! voxelctx_is_alive(ctx))
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
				}, CREATE_NODE, ctx->mgs, ctx->changed_blocks);
			}
		}
	}
}
*/

f32 voxelctx_random(Voxelctx *ctx, f32 base, f32 vary)
{
	return base + noise3d(ctx->pos.x, ctx->pos.y, ctx->pos.z, ctx->random++, seed + SO_VOXELCTX) * vary;
}
