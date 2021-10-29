#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <zlib.h>
#include <dragonport/asprintf.h>
#include "map.h"
#include "util.h"

const char *program_name;

// print system call related error message and exit
void syscall_error(const char *err)
{
	perror(err);
	exit(EXIT_FAILURE);
}

// print general error message and exit
void internal_error(const char *err)
{
	fprintf(stderr, "%s: %s\n", program_name, err);
	exit(EXIT_FAILURE);
}

// read from fd until \0 or EOF terminator
// store result including terminator into allocated buffer until bufsiz+1 is hit, return NULL on read error
char *read_string(int fd, size_t bufsiz)
{
	char buf[bufsiz + 1];
	buf[bufsiz] = 0;
	for (size_t i = 0;; i++) {
		char c;
		if (read(fd, &c, 1) == -1) {
			perror("read");
			return NULL;
		}
		if (i < bufsiz)
			buf[i] = c;
		if (c == EOF || c == 0)
			break;
	}
	return strdup(buf);
}

// convert IPv6 address to human readable, return allocated buffer
char *address_string(struct sockaddr_in6 *addr)
{
	char address[INET6_ADDRSTRLEN] = {0};
	char port[6] = {0};

	if (inet_ntop(addr->sin6_family, &addr->sin6_addr, address, INET6_ADDRSTRLEN) == NULL)
		perror("inet_ntop");
	sprintf(port, "%d", ntohs(addr->sin6_port));

	char *result = malloc(strlen(address) + 1 + strlen(port) + 1);
	sprintf(result, "%s:%s", address, port);
	return result;
}

// compress data using ZLib and store result(buffer allocated by malloc) in compressed
void my_compress(const void *uncompressed, size_t uncompressed_size, char **compressed, size_t *compressed_size)
{
	char compressed_buffer[uncompressed_size];

	z_stream stream;
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;

	stream.avail_in = stream.avail_out = uncompressed_size;
	stream.next_in = (Bytef *) uncompressed;
	stream.next_out = (Bytef *) compressed_buffer;

	deflateInit(&stream, Z_BEST_COMPRESSION);
	deflate(&stream, Z_FINISH);
	deflateEnd(&stream);

	*compressed_size = stream.total_out;
	*compressed = malloc(*compressed_size);
	memcpy(*compressed, compressed_buffer, *compressed_size);
}

// decompress data and put result into decompressed, return false if decompressed size does not match expected_decompressed_size
bool my_decompress(const char *compressed, size_t compressed_size, void *decompressed, size_t expected_decompressed_size)
{
	z_stream stream;
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;

	stream.avail_in = compressed_size;
	stream.next_in = (Bytef *) compressed;
	stream.avail_out = expected_decompressed_size;
	stream.next_out = (Bytef *) decompressed;

	inflateInit(&stream);
	inflate(&stream, Z_NO_FLUSH);
	inflateEnd(&stream);

	return (size_t) stream.total_out == expected_decompressed_size;
}

// return true if a player is close enough to a block to access it
bool within_simulation_distance(v3f64 player_pos, v3s32 block_pos, u32 simulation_distance)
{
	v3s32 player_block_pos = map_node_to_block_pos((v3s32) {player_pos.x, player_pos.y, player_pos.z}, NULL);
	return abs(player_block_pos.x - block_pos.x) <= (s32) simulation_distance && abs(player_block_pos.y - block_pos.y) <= (s32) simulation_distance && abs(player_block_pos.z - block_pos.z) <= (s32) simulation_distance;
}

f64 clamp(f64 v, f64 min, f64 max)
{
	return v < min ? min : v > max ? max : v;
}

char *format_string(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	char *ptr;
	vasprintf(&ptr, format, args);
	va_end(args);
	return ptr;
}
