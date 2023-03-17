#include "stdinc.h"
#include "shm.h"
#include "cache_api.h"
#include "target.h"
#include "mapping.h"

static struct cache* shared_cache = NULL;

int init_udm_cache(void){
    int rc = 0;
    shared_cache = alloc_shm(SHM_CACHE_NAME, sizeof(struct cache));
    if(!shared_cache) {
        perror("alloc_shm");
        rc++;
        goto err;
    }

    shared_cache->cache_dev.bdev_name = alloc_shm(SHM_BDEV_NAME, SHM_BDEV_NAME_SIZE);
    if(!shared_cache->cache_dev.bdev_name) {
        perror("alloc_shm");
        rc++;
        goto err;
    }

    /* build device: from spdk */
    strncpy(shared_cache->cache_dev.bdev_name, BDEV_NAME, SHM_BDEV_NAME_SIZE - 1);
    shared_cache->cache_dev.bdev_name[SHM_BDEV_NAME_SIZE - 1] = '\0'; // make sure string is null-terminated
    shared_cache->cache_dev.block_size = 512;
	shared_cache->cache_dev.device_size = 10000;
	shared_cache->cache_dev.cache_block_num = 1<<6;
	shared_cache->cache_dev.blocks_per_page = 8;
	shared_cache->cache_dev.blocks_per_cache_block = 64;

    rc += init_mapping(&shared_cache->cache_map, shared_cache->cache_dev.block_size, shared_cache->cache_dev.cache_block_num);
err:
    return rc;
}

int link_udm_cache(void){
    int rc = 0;
    shared_cache = alloc_shm(SHM_CACHE_NAME, sizeof(struct cache));
    if(!shared_cache) {
        perror("alloc_shm");
        rc++;
        goto err;
    }

    shared_cache->cache_dev.bdev_name = alloc_shm(SHM_BDEV_NAME, SHM_BDEV_NAME_SIZE);
    if(!shared_cache->cache_dev.bdev_name) {
        perror("alloc_shm");
        rc++;
        goto err;
    }


    if(strcmp(shared_cache->cache_dev.bdev_name, BDEV_NAME) != 0){
        printf("MSG: shared cache uninitialized\n");
        free_udm_cache();
        rc++;
        goto err;
    }

    rc += link_mapping(&shared_cache->cache_map);
err:
    return rc;
}

int free_udm_cache(void){
    if(!shared_cache) {
        printf("MSG: shared cache is null\n");
        return 1;
    }
    int rc = 0;
    rc += free_mapping(&shared_cache->cache_map);
    rc += unmap_shm(shared_cache->cache_dev.bdev_name, SHM_BDEV_NAME_SIZE);
    rc += unmap_shm(shared_cache, sizeof(struct cache));
    shared_cache = NULL;
    return rc;
}

int exit_udm_cache(void){
    int rc = 0;
    rc += exit_mapping();
    rc += unlink_shm(SHM_BDEV_NAME);
    rc += unlink_shm(SHM_CACHE_NAME);
    return rc;
}

void info_udm_cache(void){
    if(!shared_cache) {
        printf("MSG: shared cache is null\n");
        return ;
    }
    printf("---> Information of cache device <---\n");
    printf("/ bdev name = %s\n", shared_cache->cache_dev.bdev_name);
    printf("/ block_size = %u\n", shared_cache->cache_dev.block_size);
    printf("/ device_size = %u\n", shared_cache->cache_dev.device_size );
    printf("/ cache_block_num = %u\n", shared_cache->cache_dev.cache_block_num);
    printf("/ blocks_per_page = %u\n", shared_cache->cache_dev.blocks_per_page);
    printf("/ blocks_per_cache_block = %u\n", shared_cache->cache_dev.blocks_per_cache_block);
    info_mapping(&shared_cache->cache_map);
}

/* --------------------------------------------------- */

int submit_pio(struct pio* pio){
    if(!shared_cache) {
        printf("MSG: shared cache is null\n");
        return 1;
    }

    if(!pio){
        printf("MSG: pio is null\n");
        return 1;
    }

    return _submit_pio(shared_cache, pio);
}