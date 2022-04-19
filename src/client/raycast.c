#include <math.h>
#include "client/client_node.h"
#include "client/client_terrain.h"
#include "client/raycast.h"

bool raycast(v3f64 pos, v3f64 dir, f64 len, v3s32 *node_pos, NodeType *node)
{
	f64 dir_len = sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);

	while (len > 0) {
		*node = terrain_get_node(client_terrain,
			*node_pos = (v3s32) {floor(pos.x + 0.5), floor(pos.y + 0.5), floor(pos.z + 0.5)}).type;

		if (*node == NODE_UNLOADED)
			return false;

		if (client_node_defs[*node].pointable)
			return true;

		f64 vpos[3] = {pos.x, pos.y, pos.z};
		f64 vdir[3] = {dir.x, dir.y, dir.z};
		f64 min_mul = 1.0;

		for (int i = 0; i < 3; i++) {
			if (!vdir[i])
				continue;

			f64 p = vpos[i] + 0.5;
			f64 d = vdir[i] / fabs(vdir[i]);

			f64 f = floor(p) - p;
			if (d > 0.0)
				f += 1.0;
			f += 0.001 * d;

			f64 mul = f / vdir[i];
			if (min_mul > mul && mul)
				min_mul = mul;
		}

		pos = v3f64_add(pos, v3f64_scale(dir, min_mul));
		len -= dir_len * min_mul;
	}

	return false;
}
