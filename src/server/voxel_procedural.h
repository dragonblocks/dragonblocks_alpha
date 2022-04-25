#ifndef _VOXEL_PROCEDURAL_H_
#define _VOXEL_PROCEDURAL_H_

#include <dragonstd/list.h>
#include <linmath.h>
#include "server/server_terrain.h"
#include "types.h"

// Note: This is a close reimplementation of goxel procedural scripting capabilities

typedef struct {
	vec4 pos;
	vec3 scale;
	mat4x4 mat;
	f32 h, s, l;
	s32 life;
} VoxelProceduralState;

typedef struct {
	v3s32 pos;
	List *changed_chunks;
	TerrainGenStage tgs;
	s32 random;
	List state;
} VoxelProcedural;

typedef TerrainNode (*VoxelProceduralNode)(v3s32 pos, v3f32 color, void *arg);

VoxelProcedural *voxel_procedural_create(List *changed_chunks, TerrainGenStage tgs, v3s32 pos);
void voxel_procedural_delete(VoxelProcedural *proc);
void voxel_procedural_hue(VoxelProcedural *proc, f32 value);
void voxel_procedural_sat(VoxelProcedural *proc, f32 value);
void voxel_procedural_light(VoxelProcedural *proc, f32 value);
void voxel_procedural_life(VoxelProcedural *proc, s32 value);
void voxel_procedural_x(VoxelProcedural *proc, f32 value);
void voxel_procedural_y(VoxelProcedural *proc, f32 value);
void voxel_procedural_z(VoxelProcedural *proc, f32 value);
void voxel_procedural_rx(VoxelProcedural *proc, f32 value);
void voxel_procedural_ry(VoxelProcedural *proc, f32 value);
void voxel_procedural_rz(VoxelProcedural *proc, f32 value);
void voxel_procedural_sx(VoxelProcedural *proc, f32 value);
void voxel_procedural_sy(VoxelProcedural *proc, f32 value);
void voxel_procedural_sz(VoxelProcedural *proc, f32 value);
void voxel_procedural_s(VoxelProcedural *proc, f32 value);
void voxel_procedural_pop(VoxelProcedural *proc);
void voxel_procedural_push(VoxelProcedural *proc);
bool voxel_procedural_is_alive(VoxelProcedural *proc);
void voxel_procedural_cube(VoxelProcedural *proc,  VoxelProceduralNode func, void *arg);
void voxel_procedural_cylinder(VoxelProcedural *proc,  VoxelProceduralNode func, void *arg);
f32 voxel_procedural_random(VoxelProcedural *proc, f32 base, f32 vary);

#endif // _VOXEL_PROCEDURAL_H_
