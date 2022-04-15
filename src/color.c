#include "color.h"

static f32 hue_to_rgb(f32 p, f32 q, f32 t)
{
    if (t < 0.0f)
        t += 1.0f;

    if (t > 1.0f)
        t -= 1.0f;

    if (t < 1.0f / 6.0f)
        return p + (q - p) * 6.0f * t;

    if (t < 1.0f / 2.0f)
        return q;

    if (t < 2.0f / 3.0f)
        return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;

    return p;
}

v3f32 hsl_to_rgb(v3f32 hsl)
{
	v3f32 rgb;

    if (hsl.y == 0.0f) {
		rgb = (v3f32) {hsl.z, hsl.z, hsl.z};
    } else {
        f32 q = hsl.z < 0.5f ? hsl.z * (1.0f + hsl.y) : hsl.z + hsl.y - hsl.z * hsl.y;
        f32 p = 2.0f * hsl.z - q;

        rgb.x = hue_to_rgb(p, q, hsl.x + 1.0f / 3.0f);
        rgb.y = hue_to_rgb(p, q, hsl.x);
        rgb.z = hue_to_rgb(p, q, hsl.x - 1.0f / 3.0f);
    }

    return rgb;
}

