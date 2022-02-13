#ifndef _UTIL_H_
#define _UTIL_H_

#define CMPBOUNDS(x) ((x) == 0 ? 0 : (x) > 0 ? 1 : -1)                       // resolves to 1 if x > 0, 0 if x == 0 and -1 if x < 0
#define fallthrough __attribute__ ((fallthrough))                            // prevent compiler warning about implicit fallthrough with style
#define unused __attribute__ ((unused))
#define U32(x) (((u32) 1 << 31) + (x))

char *format_string(const char *format, ...);

#endif
