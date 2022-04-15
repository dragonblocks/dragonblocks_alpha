#include <math.h>
#include "physics.h"

static aabb3f64 move_box(aabb3f32 box, v3f64 pos)
{
	return (aabb3f64) {
		{pos.x + box.min.x, pos.y + box.min.y, pos.z + box.min.z},
		{pos.x + box.max.x, pos.y + box.max.y, pos.z + box.max.z},
	};
}

static aabb3s32 round_box(aabb3f64 box)
{
	return (aabb3s32) {
		{floor(box.min.x + 0.5), floor(box.min.y + 0.5), floor(box.min.z + 0.5)},
		{ ceil(box.max.x - 0.5),  ceil(box.max.y - 0.5),  ceil(box.max.z - 0.5)},
	};
}

static bool is_solid(Terrain *terrain, s32 x, s32 y, s32 z)
{
	NodeType node = terrain_get_node(terrain, (v3s32) {x, y, z}).type;
	return node == NODE_UNLOADED || node_definitions[node].solid;
}

bool physics_ground(Terrain *terrain, bool collide, aabb3f32 box, v3f64 *pos, v3f64 *vel)
{
	if (!collide)
		return false;

	if (vel->y != 0.0)
		return false;

	aabb3f64 mbox = move_box(box, *pos);
	mbox.min.y -= 0.5;

	aabb3s32 rbox = round_box(mbox);

	if (mbox.min.y - (f64) rbox.min.y > 0.01)
		return false;

	for (s32 x = rbox.min.x; x <= rbox.max.x; x++)
		for (s32 z = rbox.min.z; z <= rbox.max.z; z++)
			if (is_solid(terrain, x, rbox.min.y, z))
				return true;

	return false;
}

bool physics_step(Terrain *terrain, bool collide, aabb3f32 box, v3f64 *pos, v3f64 *vel, v3f64 *acc, f64 t)
{
	v3f64 old_pos = *pos;

	f64 *x = &pos->x;
	f64 *v = &vel->x;
	f64 *a = &acc->x;

	f32 *min = &box.min.x;
	f32 *max = &box.max.x;

	static u8 idx[3][3] = {
		{0, 1, 2},
		{1, 0, 2},
		{2, 0, 1},
	};

	for (u8 i = 0; i < 3; i++) {
		f64 v_old = v[i];
		v[i] += a[i] * t;
		f64 v_cur = (v[i] + v_old) / 2.0;
		if (v_cur == 0.0)
			continue;

		f64 x_old = x[i];
		x[i] += v_cur * t;
		if (!collide)
			continue;

		aabb3s32 box_rnd = round_box(move_box(box, *pos));

		s32 dir;
		f32 off;
		s32 *min_rnd = &box_rnd.min.x;
		s32 *max_rnd = &box_rnd.max.x;

		if (v[i] > 0.0) {
			dir = +1;
			off = max[i];

			min_rnd[i] = ceil(x_old + off + 0.5);
			max_rnd[i] = floor(x[i] + off + 0.5);
		} else {
			dir = -1;
			off = min[i];

			min_rnd[i] = floor(x_old + off - 0.5);
			max_rnd[i] =   ceil(x[i] + off - 0.5);
		}

		max_rnd[i] += dir;

		u8 i_a = idx[i][0]; // = i
		u8 i_b = idx[i][1];
		u8 i_c = idx[i][2];

		for (s32 a = min_rnd[i_a]; a != max_rnd[i_a]; a += dir)
		for (s32 b = min_rnd[i_b]; b <= max_rnd[i_b]; b++)
		for (s32 c = min_rnd[i_c]; c <= max_rnd[i_c]; c++)	{
			s32 p[3];
			p[i_a] = a;
			p[i_b] = b;
			p[i_c] = c;

			if (is_solid(terrain, p[0], p[1], p[2])) {
				x[i] = (f64) a - off - 0.5 * (f64) dir;
				v[i] = 0.0;
				goto done;
			}
		}

		done: continue;
	}

	return !v3f64_equals(*pos, old_pos);
}
