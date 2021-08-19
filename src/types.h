#ifndef _TYPES_H_
#define _TYPES_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

bool read_full(int fd, char *buffer, size_t size);

#define DEFRW(type) \
	bool read_ ## type(int fd, type *ptr); \
	bool write_ ## type(int fd, type val);

#define DEFBOX(type) \
	typedef struct {v ## type min; v ## type max;} aabb ## type;

#define DEFVEC(type) \
	typedef struct {type x, y;} v2 ## type; \
	DEFRW(v2 ## type) \
	DEFBOX(2 ## type) \
	bool v2 ## type ## _equals(v2 ## type a, v2 ## type b); \
	v2 ## type v2 ## type ## _add(v2 ## type a, v2 ## type b); \
	typedef struct {type x, y, z;} v3 ## type; \
	DEFRW(v3 ## type) \
	DEFBOX(3 ## type) \
	bool v3 ## type ## _equals(v3 ## type a, v3 ## type b); \
	v3 ## type v3 ## type ## _add(v3 ## type a, v3 ## type b);

#define DEFTYP(from, to) \
	typedef from to; \
	DEFRW(to) \
	DEFVEC(to)

#define DEFTYPES(bits) \
	DEFTYP(int ## bits ## _t, s ## bits) \
	DEFTYP(uint ## bits ## _t, u ## bits)

DEFTYPES(8)
DEFTYPES(16)
DEFTYPES(32)
DEFTYPES(64)

typedef float f32;
typedef double f64;

DEFTYP(float, f32)
DEFTYP(double, f64)

#undef DEFRW
#undef DEFBOX
#undef DEFVEC
#undef DEFTYP
#undef DEFTYPES

#endif
