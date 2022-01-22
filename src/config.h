#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stddef.h>

typedef enum
{
	CT_STRING,
	CT_INT,
	CT_UINT,
	CT_FLOAT,
	CT_BOOL,
} ConfigType;

typedef struct
{
	ConfigType type;
	char *key;
	void *value;
} ConfigEntry;

void config_read(char *path, ConfigEntry *entries, size_t num_entries);

#endif
