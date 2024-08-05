#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "common/fs.h"

char *read_file(const char *path)
{
	FILE *file = fopen(path, "r");
	if (!file) {
		perror("fopen");
		return NULL;
	}

	if (fseek(file, 0, SEEK_END) == -1) {
		perror("fseek");
		fclose(file);
		return NULL;
	}

	long size = ftell(file);

	if (size == -1) {
		perror("ftell");
		fclose(file);
		return NULL;
	}

	if (fseek(file, 0, SEEK_SET) == -1) {
		perror("fseek");
		fclose(file);
		return NULL;
	}

	char *buffer = malloc(size+1);
	buffer[size] = '\0';

	if (fread(buffer, 1, size, file) != (size_t) size) {
		perror("fread");
		fclose(file);
		free(buffer);
		return NULL;
	}

	fclose(file);
	return buffer;
}

bool directory_exists(const char *path)
{
	// stat() does not properly handle trailing slashes on dirs on MinGW in some cases
	// strip trailing slashes
	size_t len = strlen(path);
	char stat_path[len+1];
	strcpy(stat_path, path);
	while (len > 0 && stat_path[len-1] == '/')
		stat_path[--len] = '\0';

	struct stat sb;
	return stat(stat_path, &sb) == 0 && S_ISDIR(sb.st_mode);
}
