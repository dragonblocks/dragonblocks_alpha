#define _GNU_SOURCE // don't worry, GNU extensions are only used when available
#include <dragonnet/init.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "common/fs.h"

void dragonblocks_init()
{
#ifdef __GLIBC__ // check whether bloat is enabled
	pthread_setname_np(pthread_self(), "main");
#endif // __GLIBC__

	if (!directory_exists(ASSET_PATH)) {
		fprintf(stderr, "[error] asset directory not found at %s, "
			"invoke game from correct path\n", ASSET_PATH);
		exit(EXIT_FAILURE);
	}

	dragonnet_init();
}
