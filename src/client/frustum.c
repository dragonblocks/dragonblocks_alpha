#include "client/camera.h"
#include "client/frustum.h"
#include "client/window.h"

typedef enum {
	PLANE_LEFT,
	PLANE_RIGHT,
	PLANE_BOTTOM,
	PLANE_TOP,
	PLANE_NEAR,
	PLANE_FAR,
	PLANE_COUNT,
} Plane;

mat4x4 frustum;

static vec3 points[8];
static vec4 planes[PLANE_COUNT];
static int cross_indices[PLANE_COUNT][PLANE_COUNT];

__attribute__((constructor)) static void frustum_init()
{
	for (Plane a = 0; a < PLANE_COUNT; a++)
		for (Plane b = 0; b < PLANE_COUNT; b++)
			cross_indices[a][b] = a * (9 - a) / 2 + b - 1;
}

void frustum_update()
{
	mat4x4_mul(frustum, window.projection, camera.view);

	mat4x4 m;
	mat4x4_transpose(m, frustum);

	vec4_add(planes[PLANE_LEFT], m[3], m[0]);
	vec4_sub(planes[PLANE_RIGHT], m[3], m[0]);
	vec4_add(planes[PLANE_BOTTOM], m[3], m[1]);
	vec4_sub(planes[PLANE_TOP], m[3], m[1]);
	vec4_add(planes[PLANE_NEAR], m[3], m[2]);
	vec4_sub(planes[PLANE_FAR], m[3], m[2]);

	int i = 0;
	vec3 crosses[PLANE_COUNT * (PLANE_COUNT - 1) / 2];
	for (Plane a = 0; a < PLANE_COUNT; a++)
		for (Plane b = a + 1; b < PLANE_COUNT; b++)
			vec3_mul_cross(crosses[i++], planes[a], planes[b]);

	int j = 0;
	for (Plane c = PLANE_NEAR; c <= PLANE_FAR; c++) {
		for (Plane a = PLANE_LEFT; a <= PLANE_RIGHT; a++) {
			for (Plane b = PLANE_BOTTOM; b <= PLANE_TOP; b++) {
				float d = -1.0f / vec3_mul_inner(planes[a], crosses[cross_indices[b][c]]);
				vec3 w = {planes[a][3], planes[b][3], planes[c][3]};
				float *res = points[j++];

				vec3 res_1_cross = {-crosses[cross_indices[a][c]][0], -crosses[cross_indices[a][c]][1], -crosses[cross_indices[a][c]][2]};

				res[0] = vec3_mul_inner(crosses[cross_indices[b][c]], w) * d;
				res[1] = vec3_mul_inner(res_1_cross, w) * d;
				res[2] = vec3_mul_inner(crosses[cross_indices[a][b]], w) * d;
			}
		}
	}
}

static bool outside_plane(Plane i, aabb3f32 box)
{
	for (int x = 0; x <= 1; x++) {
		for (int y = 0; y <= 1; y++) {
			for (int z = 0; z <= 1; z++) {
				vec4 plane = {
					x ? box.max.x : box.min.x,
					y ? box.max.y : box.min.y,
					z ? box.max.z : box.min.z,
					1.0f,
				};

				if (vec4_mul_inner(planes[i], plane) > 0.0)
					return false;
			}
		}
	}

	return true;
}

// http://iquilezles.org/www/articles/frustumcorrect/frustumcorrect.htm
static bool box_visible(aabb3f32 box)
{
	for (Plane i = 0; i < PLANE_COUNT; i++) {
		if (outside_plane(i, box))
			return false;
	}

	int box_outside[6] = {0};

	for (Plane i = 0; i < PLANE_COUNT; i++) {
		int outside[6] = {
			points[i][0] > box.max.x,
			points[i][0] < box.min.x,
			points[i][1] > box.max.y,
			points[i][1] < box.min.y,
			points[i][2] > box.max.z,
			points[i][2] < box.min.z,
		};

		for (int i = 0; i < 6; i++)
			box_outside[i] += outside[i];
	}

	for (int i = 0; i < 6; i++) {
		if (box_outside[i] == 8)
			return false;
	}

	return true;
}

bool frustum_cull(aabb3f32 box)
{
	return !box_visible(box);
}
