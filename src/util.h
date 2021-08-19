#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdbool.h>
#include <arpa/inet.h>
#include "types.h"

#define ever (;;)														// infinite for loop with style
#define INBRACES(str) str ? "(" : "", str ? str : "", str ? ")" : ""	// wrapper for printf to optionally add a message in braces if message is not NULL
#define CMPBOUNDS(x) x == 0 ? 0 : x > 0 ? 1 : -1						// resolves 1 if x > 0, 0 if x == 0 and -1 if x < 0
#define fallthrough __attribute__ ((fallthrough))						// prevent compiler warning about implicit fallthrough with style

extern const char *program_name;	// this has to be set to program name on startup

void syscall_error(const char *err);																						// print system call related error message and exit
void internal_error(const char *err);																						// print general error message and exit
char *read_string(int fd, size_t bufsiz);																					// read from fd until \0 or EOF terminator
char *address_string(struct sockaddr_in6 *addr);																			// convert IPv6 address to human readable, return allocated buffer
v3f32 html_to_v3f32(const char *html);																						// convert #RRGGBB color to v3f32
void my_compress(const void *uncompressed, size_t uncompressed_size, char **compressed, size_t *compressed_size);			// compress data using ZLib and store result(buffer allocated by malloc) in compressed
bool my_decompress(const char *compressed, size_t compressed_size, void *decompressed, size_t expected_decompressed_size);	// decompress data and put result into decompressed, return false if decompressed size does not match expected_decompressed_size
bool within_simulation_distance(v3f64 player_pos, v3s32 block_pos, u32 simulation_distance);								// return true if a player is close enough to a block to access it

#endif
