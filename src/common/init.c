#define _GNU_SOURCE // don't worry, GNU extensions are only used when available
#include <dragonnet/init.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

void dragonblocks_init(int argc)
{
#ifdef __GLIBC__ // check whether bloat is enabled
	pthread_setname_np(pthread_self(), "main");
#endif // __GLIBC__

	// stat() does not properly handle trailing slashes on dirs on MinGW in some cases
	// strip trailing slashes
	size_t len = strlen(ASSET_PATH);
	char stat_path[len+1];
	strcpy(stat_path, ASSET_PATH);
	while (len > 0 && stat_path[len-1] == '/')
		stat_path[--len] = '\0';

	struct stat sb;
	if (stat(stat_path, &sb) != 0 || !S_ISDIR(sb.st_mode)) {
		fprintf(stderr, "[error] asset directory not found at %s, "
			"invoke game from correct path\n", ASSET_PATH);
		exit(EXIT_FAILURE);
	}

	if (argc < 2) {
		fprintf(stderr, "[error] missing address\n");
		exit(EXIT_FAILURE);
	}

	dragonnet_init();
}
