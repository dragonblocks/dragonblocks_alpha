#ifndef _TYPES_H_
#define _TYPES_H_

#include <stdint.h>
#include <stdbool.h>

#define DEFRW(type) \
	bool read_ ## type(int fd, type *ptr); \
	bool write_ ## type(int fd, type val);

#define DEFVEC(type) \
	typedef struct {type x, y;} v2 ## type; \
	DEFRW(v2 ## type) \
	typedef struct {type x, y, z;} v3 ## type; \
	DEFRW(v3 ## type)

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

#undef DEFRW
#undef DEFVEC
#undef DEFTYP
#undef DEFTYPES

#endif
