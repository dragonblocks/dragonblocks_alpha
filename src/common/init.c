#define _GNU_SOURCE // don't worry, GNU extensions are only used when available
#include <dragonnet/init.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

void dragonblocks_init(int argc)
{
#ifdef __GLIBC__ // check whether bloat is enabled
	pthread_setname_np(pthread_self(), "main");
#endif // __GLIBC__

	struct stat sb;
	if (stat(ASSET_PATH, &sb) != 0 || !S_ISDIR(sb.st_mode)) {
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
