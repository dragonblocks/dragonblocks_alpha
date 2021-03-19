#ifndef _TYPES_H_
#define _TYPES_H_

#include <stdint.h>

#define DEFVEC(type) typedef struct {type x, y;} v2 ## type; typedef struct {type x, y, z;} v3 ## type;
#define DEFTYP(from, to) typedef from to; DEFVEC(to)
#define DEFTYPES(bytes) DEFTYP(int ## bytes ## _t, s ## bytes) DEFTYP(uint ## bytes ## _t, u ## bytes)

DEFTYPES(8)
DEFTYPES(16)
DEFTYPES(32)
DEFTYPES(64)

typedef float f32;
typedef double f64;

DEFVEC(f32)
DEFVEC(f64)

typedef v2f32 v2f;
typedef v3f32 v3f;

#undef DEFVEC
#undef DEFTYP
#undef DEFTYPES

#endif
