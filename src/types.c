#include <stdio.h>
#include <unistd.h>
#include <endian.h>
#include <poll.h>
#include "types.h"

bool read_full(int fd, char *buffer, size_t size)
{
	size_t n_read_total = 0;
	int n_read;
	while (n_read_total < size) {
		if ((n_read = read(fd, buffer + n_read_total, size - n_read_total)) == -1) {
			perror("read");
			return false;
		}
		n_read_total += n_read;
	}
	return true;
}

#define htobe8(x) x
#define be8toh(x) x

#define READVEC(type, n) \
	type buf[n]; \
	for (int i = 0; i < n; i++) { \
		if (! read_ ## type(fd, &buf[i])) \
			return false; \
	}

#define WRITEVEC(type, n) \
	for (int i = 0; i < n; i++) { \
		if (! write_ ## type(fd, vec[i])) \
			return false; \
	} \
	return true;

#define DEFVEC(type) \
	bool read_v2 ## type(int fd, v2 ## type *ptr) \
	{ \
		READVEC(type, 2) \
		ptr->x = buf[0]; \
		ptr->y = buf[1]; \
		return true; \
	} \
	bool write_v2 ## type(int fd, v2 ## type val) \
	{ \
		type vec[2] = {val.x, val.y}; \
		WRITEVEC(type, 2) \
	} \
	bool v2 ## type ## _equals(v2 ## type a, v2 ## type b) \
	{ \
		return a.x == b.x && a.y == b.y; \
	} \
	v2 ## type v2 ## type ## _add(v2 ## type a, v2 ## type b) \
	{ \
		return (v2 ## type) {a.x + b.x, a.y + b.y}; \
	} \
	bool read_v3 ## type(int fd, v3 ## type *ptr) \
	{ \
		READVEC(type, 3) \
		ptr->x = buf[0]; \
		ptr->y = buf[1]; \
		ptr->z = buf[2]; \
		return true; \
	} \
	bool write_v3 ## type(int fd, v3 ## type val) \
	{ \
		type vec[3] = {val.x, val.y, val.z}; \
		WRITEVEC(type, 3) \
	} \
	bool v3 ## type ## _equals(v3 ## type a, v3 ## type b) \
	{ \
		return a.x == b.x && a.y == b.y && a.z == b.z; \
	} \
	v3 ## type v3 ## type ## _add(v3 ## type a, v3 ## type b) \
	{ \
		return (v3 ## type) {a.x + b.x, a.y + b.y, a.z + b.z}; \
	}

#define DEFTYP(type, bits) \
	bool read_ ## type(int fd, type *buf) \
	{ \
		u ## bits encoded; \
		if (! read_full(fd, (char *) &encoded, sizeof(type))) \
			return false; \
		*buf = be ## bits ## toh(encoded); \
		return true; \
	} \
	bool write_ ## type(int fd, type val) \
	{ \
		u ## bits encoded = htobe ## bits(val); \
		if (write(fd, &encoded, sizeof(encoded)) == -1) { \
			perror("write"); \
			return false; \
		} \
		return true; \
	} \
	DEFVEC(type)

#define DEFTYPES(bits) \
	DEFTYP(s ## bits, bits) \
	DEFTYP(u ## bits, bits)

DEFTYPES(8)
DEFTYPES(16)
DEFTYPES(32)
DEFTYPES(64)

#define DEFFLOAT(type) \
	bool read_ ## type(int fd, type *buf) \
	{ \
		if (! read_full(fd, (char *) buf, sizeof(type))) \
			return false; \
		return true; \
	} \
	bool write_ ## type(int fd, type val) \
	{ \
		if (write(fd, &val, sizeof(val)) == -1) { \
			perror("write"); \
			return false; \
		} \
		return true; \
	} \
	DEFVEC(type)

DEFFLOAT(f32)
DEFFLOAT(f64)
