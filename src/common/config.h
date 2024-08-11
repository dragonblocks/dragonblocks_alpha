#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stddef.h>

typedef enum {
	CONFIG_STRING,
	CONFIG_INT,
	CONFIG_UINT,
	CONFIG_FLOAT,
	CONFIG_BOOL,
} ConfigType;

typedef struct {
	ConfigType type;
	char *key;
	void *value;
} ConfigEntry;

void config_read(const char *path, ConfigEntry *entries, size_t num_entries);
void config_free(ConfigEntry *entries, size_t num_entires);

#endif // _CONFIG_H_
