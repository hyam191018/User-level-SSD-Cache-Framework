#include "spdk.h"

device_info get_device_info(void) { printf("%d, %d\n", BLOCKS_PER_PAGE, BLOCKS_PER_CACHE_BLOCK); }