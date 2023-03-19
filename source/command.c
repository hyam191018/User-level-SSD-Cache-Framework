#include "cache_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void setup(void) {
	init_udm_cache();
	info_udm_cache();
	free_udm_cache();
}

static void reset(void) {
	exit_udm_cache();
}

static void user(void) {
	printf("I am user!\n");
	link_udm_cache();
	info_udm_cache();
	free_udm_cache();
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s [command]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "setup") == 0) {
        setup();
    } else if (strcmp(argv[1], "reset") == 0) {
        reset();
    } else if (strcmp(argv[1], "user") == 0) {
        user();
    } else {
        printf("Invalid argument: %s\n", argv[1]);
        return 1;
    }

    return 0;
}