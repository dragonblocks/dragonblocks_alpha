#ifndef _UTIL_H_
#define _UTIL_H_

#define ever (;;)                                                            // infinite for loop with style
#define INBRACES(str) (str) ? "(" : "", (str) ? (str) : "", (str) ? ")" : "" // wrapper for printf to optionally add a message in braces if message is not NULL
#define CMPBOUNDS(x) ((x) == 0 ? 0 : (x) > 0 ? 1 : -1)                       // resolves to 1 if x > 0, 0 if x == 0 and -1 if x < 0
#define fallthrough __attribute__ ((fallthrough))                            // prevent compiler warning about implicit fallthrough with style
#define unused __attribute__ ((unused))
#define U32(x) (((u32) 1 << 31) + (x))

char *format_string(const char *format, ...);

#endif
