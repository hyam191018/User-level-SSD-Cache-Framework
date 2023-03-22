#include "cache_api.h"
#include "mapping.h"
#include "shm.h"
#include "stdinc.h"
#include "target.h"

static struct cache *shared_cache = NULL;

static void *migration(void *arg) {
    struct timespec ts = {0, MIGRATION_DELAY};
    while (1) {
        if (!do_migration_work(&shared_cache->cache_map)) {
            // 沒事做的話，檢查是否有取消請求
            pthread_testcancel();
            nanosleep(&ts, NULL);
        }
    }
    return NULL;
}

static int wakeup_mg_worker(void) {
    if (!shared_cache) {
        printf("Error: shared cache is null\n");
        return 1;
    }
    if (shared_cache->mg_worker != 0) {
        printf("Error: mg_worker is running\n");
        return 1;
    }
    return pthread_create(&shared_cache->mg_worker, NULL, &migration, NULL);
}

static int shutdown_mg_worker(void) {
    if (!shared_cache) {
        printf("Error: shared cache is null\n");
        return 1;
    }
    if (shared_cache->mg_worker == 0) {
        printf("Error: mg_worker is not running\n");
        return 1;
    }
    pthread_cancel(shared_cache->mg_worker);
    int rc = pthread_join(shared_cache->mg_worker, NULL);
    shared_cache->mg_worker = 0;
    return rc;
}

static void *writeback(void *arg) {
    struct timespec ts = {0, WRITEBACK_DELAY};
    while (1) {
        if (!do_writeback_work(&shared_cache->cache_map)) {
            // 沒事做的話，檢查是否有取消請求
            pthread_testcancel();
            nanosleep(&ts, NULL);
        }
    }
    return NULL;
}

static int wakeup_wb_worker(void) {
    if (!shared_cache) {
        printf("Error: shared cache is null\n");
        return 1;
    }
    if (shared_cache->wb_worker != 0) {
        printf("Error: wb_worker is running\n");
        return 1;
    }

    return pthread_create(&shared_cache->wb_worker, NULL, &writeback, NULL);
}

static int shutdown_wb_worker(void) {
    if (!shared_cache) {
        printf("Error: shared cache is null\n");
        return 1;
    }
    if (shared_cache->wb_worker == 0) {
        printf("Error: wb_worker is not running\n");
        return 1;
    }

    pthread_cancel(shared_cache->wb_worker);
    int res = pthread_join(shared_cache->wb_worker, NULL);
    shared_cache->wb_worker = 0;
    return res;
}

/* --------------------------------------------------- */

int init_udm_cache(void) {
    shared_cache = alloc_shm(SHM_CACHE_NAME, sizeof(struct cache));
    if (!shared_cache) {
        return 1;
    }
    shared_cache->cache_dev.bdev_name = alloc_shm(SHM_BDEV_NAME, SHM_BDEV_NAME_SIZE);
    if (!shared_cache->cache_dev.bdev_name) {
        return 1;
    }
    if (shared_cache->cache_state.running) {
        printf("Error: init_udm_cache - admin is running\n");
        return 1;
    }

    /* build device: from spdk */
    strncpy(shared_cache->cache_dev.bdev_name, BDEV_NAME, SHM_BDEV_NAME_SIZE - 1);
    shared_cache->cache_dev.bdev_name[SHM_BDEV_NAME_SIZE - 1] =
        '\0';  // make sure string is null-terminated
    shared_cache->cache_dev.block_size = 512;
    shared_cache->cache_dev.device_size = 10000;
    shared_cache->cache_dev.cache_block_num = CACHE_BLOCK_NUMBER;
    shared_cache->cache_dev.blocks_per_page = 8;
    shared_cache->cache_dev.blocks_per_cache_block = 64;
    if (shared_cache->cache_dev.block_size * shared_cache->cache_dev.device_size <
        CACHE_BLOCK_NUMBER * CACHE_BLOCK_SIZE) {
        printf("Error: Cache device size is not enough for setting cblock number\n");
        return 1;
    }
    if (init_mapping(&shared_cache->cache_map, shared_cache->cache_dev.block_size,
                     shared_cache->cache_dev.cache_block_num)) {
        return 1;
    }
    wakeup_mg_worker();
    wakeup_wb_worker();
    spinlock_init(&shared_cache->cache_state.lock);
    shared_cache->cache_state.running = true;
    shared_cache->cache_state.count = 0; /* admin */
    return 0;
}

int link_udm_cache(void) {
    shared_cache = alloc_shm(SHM_CACHE_NAME, sizeof(struct cache));
    if (!shared_cache) {
        return 1;
    }
    if (!shared_cache->cache_state.running) {
        printf("Error: link_udm_cache - shared cache uninitialized\n");
        return 1;
    }
    shared_cache->cache_dev.bdev_name = alloc_shm(SHM_BDEV_NAME, SHM_BDEV_NAME_SIZE);
    if (!shared_cache->cache_dev.bdev_name) {
        return 1;
    }
    if (link_mapping(&shared_cache->cache_map)) {
        return 1;
    }
    if (!shared_cache->cache_state.running) {
        printf("Error: link_udm_cache - shared cache uninitialized\n");
        unmap_shm(shared_cache, sizeof(struct cache));
        return 1;
    }
    spinlock_lock(&shared_cache->cache_state.lock);
    shared_cache->cache_state.count++;
    spinlock_unlock(&shared_cache->cache_state.lock);
    return 0;
}

int free_udm_cache(void) {
    if (!shared_cache) {
        printf("Error: free_udm_cache - shared cache uninitialized\n");
        return 1;
    }
    if (!shared_cache->cache_state.running) {
        printf("Error: free_udm_cache - shared cache uninitialized\n");
        return 1;
    }
    int rc = 0;
    spinlock_lock(&shared_cache->cache_state.lock);
    shared_cache->cache_state.count--;
    spinlock_unlock(&shared_cache->cache_state.lock);
    rc += free_mapping(&shared_cache->cache_map);
    rc += unmap_shm(shared_cache->cache_dev.bdev_name, SHM_BDEV_NAME_SIZE);
    rc += unmap_shm(shared_cache, sizeof(struct cache));
    return rc;
}

int exit_udm_cache(void) {
    if (!shared_cache) {
        printf("Error: exit_udm_cache - shared cache uninitialized\n");
        return 1;
    }
    shutdown_wb_worker();
    shutdown_mg_worker();
    while (true) {
        spinlock_lock(&shared_cache->cache_state.lock);
        if (!shared_cache->cache_state.count) {
            spinlock_unlock(&shared_cache->cache_state.lock);
            break;
        }
        printf("Waiting for %d user(s) ... \n", shared_cache->cache_state.count);
        spinlock_unlock(&shared_cache->cache_state.lock);
        sleep(1);
    }
    int rc = 0;
    rc += free_mapping(&shared_cache->cache_map);
    rc += unmap_shm(shared_cache->cache_dev.bdev_name, SHM_BDEV_NAME_SIZE);
    rc += unmap_shm(shared_cache, sizeof(struct cache));
    rc += exit_mapping();
    rc += unlink_shm(SHM_BDEV_NAME);
    rc += unlink_shm(SHM_CACHE_NAME);
    return rc;
}

void force_exit_udm_cache(void) {
    shutdown_wb_worker();
    shutdown_mg_worker();
    exit_mapping();
    unlink_shm(SHM_BDEV_NAME);
    unlink_shm(SHM_CACHE_NAME);
}

void info_udm_cache(void) {
    if (!shared_cache) {
        printf("Error: info_udm_cache - shared cache uninitialized\n");
        return;
    }
    if (!shared_cache->cache_state.running) {
        printf("Error: info_udm_cache - shared cache uninitialized\n");
        return;
    }
    printf("---> Information of cache device <---\n");
    printf("/ bdev name = %s\n", shared_cache->cache_dev.bdev_name);
    printf("/ block_size = %u\n", shared_cache->cache_dev.block_size);
    printf("/ device_size = %u\n", shared_cache->cache_dev.device_size);
    printf("/ cache_block_num = %u\n", shared_cache->cache_dev.cache_block_num);
    printf("/ blocks_per_page = %u\n", shared_cache->cache_dev.blocks_per_page);
    printf("/ blocks_per_cache_block = %u\n", shared_cache->cache_dev.blocks_per_cache_block);
    info_mapping(&shared_cache->cache_map);
}

/* --------------------------------------------------- */

int submit_pio(struct pio *pio) {
    if (!shared_cache) {
        printf("Error: shared cache is null\n");
        return 1;
    }
    if (!shared_cache->cache_state.running) {
        printf("Error: admin is not running\n");
        return 1;
    }
    if (!pio) {
        printf("Error: pio is null\n");
        return 1;
    }

    return _submit_pio(shared_cache, pio);
}