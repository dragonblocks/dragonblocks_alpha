#include <stdlib.h>
#include <pthread.h>
#include "color.h"
#include "perlin.h"
#include "server/terrain_gen.h"
#include "server/voxel_procedural.h"

#define VOXEL_PROCEDURAL_STATE(proc) (*((VoxelProceduralState *) (proc)->state.fst->dat))

static VoxelProceduralState *create_state(VoxelProceduralState *old)
{
	VoxelProceduralState *state = malloc(sizeof *state);

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

VoxelProcedural *voxel_procedural_create(List *changed_chunks, TerrainGenStage tgs, v3s32 pos)
{
	VoxelProcedural *proc = malloc(sizeof(VoxelProcedural));

	proc->changed_chunks = changed_chunks;
	proc->tgs = tgs;
	proc->pos = pos;
	proc->random = 0;

	list_ini(&proc->state);
	list_apd(&proc->state, create_state(NULL));

	return proc;
}

void voxel_procedural_delete(VoxelProcedural *proc)
{
	list_clr(&proc->state, &free, NULL, NULL);
	free(proc);
}

static void move_value(f32 *x, f32 v, f32 range)
{
    f32 dst = v >= 0 ? range : 0;
    v = fabs(v);
    *x = f32_mix(*x, dst, v);
}

void voxel_procedural_hue(VoxelProcedural *proc, f32 value)
{
	VOXEL_PROCEDURAL_STATE(proc).h += value;
}

void voxel_procedural_sat(VoxelProcedural *proc, f32 value)
{
	move_value(&VOXEL_PROCEDURAL_STATE(proc).s, value, 1.0f);
}

void voxel_procedural_light(VoxelProcedural *proc, f32 value)
{
	move_value(&VOXEL_PROCEDURAL_STATE(proc).l, value, 1.0f);
}

void voxel_procedural_life(VoxelProcedural *proc, s32 value)
{
	VOXEL_PROCEDURAL_STATE(proc).life += value;
}

static void apply_translation(VoxelProcedural *proc, v3f32 translate)
{
	vec4 dst, src = {translate.x, translate.y, translate.z, 0.0f};
	mat4x4_mul_vec4(dst, VOXEL_PROCEDURAL_STATE(proc).mat, src);
	vec4_add(VOXEL_PROCEDURAL_STATE(proc).pos, VOXEL_PROCEDURAL_STATE(proc).pos, dst);
}

void voxel_procedural_x(VoxelProcedural *proc, f32 value)
{
	apply_translation(proc, (v3f32) {value, 0.0f, 0.0f});
}

void voxel_procedural_y(VoxelProcedural *proc, f32 value)
{
	apply_translation(proc, (v3f32) {0.0f, value, 0.0f});
}

void voxel_procedural_z(VoxelProcedural *proc, f32 value)
{
	apply_translation(proc, (v3f32) {0.0f, 0.0f, value});
}

void voxel_procedural_rx(VoxelProcedural *proc, f32 value)
{
	mat4x4_rotate_X(VOXEL_PROCEDURAL_STATE(proc).mat, VOXEL_PROCEDURAL_STATE(proc).mat,
		value * M_PI / 180.0f);
}

void voxel_procedural_ry(VoxelProcedural *proc, f32 value)
{
	mat4x4_rotate_Y(VOXEL_PROCEDURAL_STATE(proc).mat, VOXEL_PROCEDURAL_STATE(proc).mat,
		value * M_PI / 180.0f);
}

void voxel_procedural_rz(VoxelProcedural *proc, f32 value)
{
	mat4x4_rotate_Z(VOXEL_PROCEDURAL_STATE(proc).mat, VOXEL_PROCEDURAL_STATE(proc).mat,
		value * M_PI / 180.0f);
}

static void apply_scale(VoxelProcedural *proc, v3f32 scale)
{
	VOXEL_PROCEDURAL_STATE(proc).scale[0] *= scale.x;
	VOXEL_PROCEDURAL_STATE(proc).scale[1] *= scale.y;
	VOXEL_PROCEDURAL_STATE(proc).scale[2] *= scale.z;

	mat4x4_scale_aniso(VOXEL_PROCEDURAL_STATE(proc).mat, VOXEL_PROCEDURAL_STATE(proc).mat,
		scale.x, scale.y, scale.z);
}

void voxel_procedural_sx(VoxelProcedural *proc, f32 value)
{
	apply_scale(proc, (v3f32) {value, 1.0f, 1.0f});
}

void voxel_procedural_sy(VoxelProcedural *proc, f32 value)
{
	apply_scale(proc, (v3f32) {1.0f, value, 1.0f});
}

void voxel_procedural_sz(VoxelProcedural *proc, f32 value)
{
	apply_scale(proc, (v3f32) {1.0f, 1.0f, value});
}

void voxel_procedural_s(VoxelProcedural *proc, f32 value)
{
	apply_scale(proc, (v3f32) {value, value, value});
}

void voxel_procedural_pop(VoxelProcedural *proc)
{
	free(proc->state.fst->dat);
	list_nrm(&proc->state, &proc->state.fst);
}

void voxel_procedural_push(VoxelProcedural *proc)
{
	list_ppd(&proc->state, create_state(&VOXEL_PROCEDURAL_STATE(proc)));
}

bool voxel_procedural_is_alive(VoxelProcedural *proc)
{
	if (VOXEL_PROCEDURAL_STATE(proc).life > 0 && --VOXEL_PROCEDURAL_STATE(proc).life <= 0)
		return false;

	return
		VOXEL_PROCEDURAL_STATE(proc).scale[0] >= 1.0f &&
		VOXEL_PROCEDURAL_STATE(proc).scale[1] >= 1.0f &&
		VOXEL_PROCEDURAL_STATE(proc).scale[2] >= 1.0f;
}

void voxel_procedural_cube(VoxelProcedural *proc, NodeType node, bool use_color)
{
	if (!voxel_procedural_is_alive(proc))
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

	mat4x4_mul_vec4(center, VOXEL_PROCEDURAL_STATE(proc).mat, (vec4) {0.5f, 0.5f, 0.5f});

	for (int i = 0; i < 8; i++) {
		mat4x4_mul_vec4(corners[i], VOXEL_PROCEDURAL_STATE(proc).mat, base_corners[i]);

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

			v[i] = floor(VOXEL_PROCEDURAL_STATE(proc).pos[i] + f + 0.5f);
		}

		Blob buffer = {0, NULL};

		if (use_color)
			ColorData_write(&buffer, &(ColorData) {hsl_to_rgb((v3f32) {
				VOXEL_PROCEDURAL_STATE(proc).h / 360.0,
				VOXEL_PROCEDURAL_STATE(proc).s,
				VOXEL_PROCEDURAL_STATE(proc).l,
			})});

		server_terrain_gen_node(
			v3s32_add(proc->pos, (v3s32) {v[0], v[2], v[1]}),
			terrain_node_create(node, buffer),
			proc->tgs,
			proc->changed_chunks
		);

		Blob_free(&buffer);
	}
}


void voxel_procedural_cylinder(VoxelProcedural *proc, NodeType node, bool use_color)
{
	voxel_procedural_cube(proc, node, use_color);
}

/*
void voxel_procedural_cylinder(VoxelProcedural *proc, Node node, bool use_color)
{
	if (!voxel_procedural_is_alive(proc))
		return;

	return;

	f32 xf = VOXEL_PROCEDURAL_STATE(proc).scale[0] / 2.0f;
	for (s32 x = round(-xf + 0.5f); x <= round(xf); x++) {
		f32 yf = cos(x / VOXEL_PROCEDURAL_STATE(proc).scale[0] * M_PI) * VOXEL_PROCEDURAL_STATE(proc).scale[1] / 2.0f;
		for (s32 y = round(-yf); y <= round(yf); y++) {
			f32 zf = VOXEL_PROCEDURAL_STATE(proc).scale[2] / 2.0f;
			for (s32 z = round(-zf + 0.5f); z <= round(zf); z++) {
				mapgen_set_node((v3s32) {
					proc->pos.x + round(VOXEL_PROCEDURAL_STATE(proc).pos[0] + x),
					proc->pos.y + round(VOXEL_PROCEDURAL_STATE(proc).pos[2] + z),
					proc->pos.z + round(VOXEL_PROCEDURAL_STATE(proc).pos[1] + y),
				}, CREATE_NODE, proc->tgs, proc->changed_chunks);
			}
		}
	}
}
*/

f32 voxel_procedural_random(VoxelProcedural *proc, f32 base, f32 vary)
{
	return base + noise3d(proc->pos.x, proc->pos.y, proc->pos.z, proc->random++, seed + OFFSET_VOXEL_PROCEDURAL) * vary;
}
