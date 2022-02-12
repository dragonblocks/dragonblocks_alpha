#include <stdarg.h>
#include <dragonport/asprintf.h>

char *format_string(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	char *ptr;
	vasprintf(&ptr, format, args);
	va_end(args);
	return ptr;
}
