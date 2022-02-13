#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "config.h"

void config_read(char *path, ConfigEntry *entries, size_t num_entries)
{
	FILE *f = fopen(path, "r");

	if (! f)
		return;

	printf("Reading config from %s\n", path);

	while (! feof(f)) {
		char key[BUFSIZ];
		if (! fscanf(f, "%s", key))
			break;

		bool found = false;
		for (size_t i = 0; i < num_entries; i++) {
			ConfigEntry *entry = &entries[i];
			if (strcmp(key, entry->key) == 0) {
				bool valid = false;

				switch (entry->type) {
					case CT_STRING: {
						char value[BUFSIZ];

						if (! fscanf(f, "%s", value))
							break;

						valid = true;
						*(char **) entry->value = strdup(value);
					} break;

					case CT_INT:
						if (fscanf(f, "%d", (int *) entry->value))
							valid = true;

						break;

					case CT_UINT:
						if (fscanf(f, "%u", (unsigned int *) entry->value))
							valid = true;

						break;

					case CT_FLOAT:
						if (fscanf(f, "%lf", (double *) entry->value))
							valid = true;

						break;

					case CT_BOOL: {
						char value[BUFSIZ];

						if (! fscanf(f, "%s", value))
							break;

						valid = true;

						if (strcmp(value, "on") == 0 || strcmp(value, "yes") == 0 || strcmp(value, "true") == 0)
							*(bool *) entry->value = true;
						else if (strcmp(value, "off") == 0 || strcmp(value, "no") == 0 || strcmp(value, "false") == 0)
							*(bool *) entry->value = false;
						else
							valid = false;

					} break;
				}

				if (! valid)
					fprintf(stderr, "Invalid value for setting %s in %s\n", key, path);

				found = true;
				break;
			}
		}

		if (! found)
			fprintf(stderr, "Unknown setting %s in %s\n", key, path);
	}

	fclose(f);
}
