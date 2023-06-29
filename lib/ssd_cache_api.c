#include "cache_api.h"
#include "config.h"
#include "mapping.h"
#include "shm.h"
#include "spdk.h"
#include "stdinc.h"
#include "target.h"

static struct cache *shared_cache = NULL;

static void *migration(void *arg) {
    struct timespec ts = {0, MIGRATION_DELAY};
    void *dma_buf = alloc_dma_buffer(CACHE_BLOCK_SIZE);
    while (1) {
        if (!do_migration_work(&shared_cache->cache_map, dma_buf)) {
            // 沒事做的話，檢查是否有取消請求
            if (is_empty(&shared_cache->cache_map.wq)) {
                pthread_testcancel();
                nanosleep(&ts, NULL);
            }
        }
    }
    free_dma_buffer(dma_buf);
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
/*
static void *writeback(void *arg) {
    struct timespec ts = {0, WRITEBACK_DELAY};
    void *dma_buf = alloc_dma_buffer(CACHE_BLOCK_SIZE);
    while (true) {
        if (!do_writeback_work(&shared_cache->cache_map, dma_buf)) {
            // 沒事做的話，檢查是否有取消請求
            pthread_testcancel();
            nanosleep(&ts, NULL);
        }
    }
    free_dma_buffer(dma_buf);
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
*/
/* --------------------------------------------------- */

static inline int LOG2(unsigned int n) { return (n > 1) ? (1 + LOG2(n >> 1)) : 0; }

int init_ssd_cache(void) {
    shared_cache = alloc_shm(SHM_CACHE_NAME, sizeof(struct cache));
    if (!shared_cache) {
        return 1;
    }
    if (shared_cache->cache_state.running) {
        printf("Error: init_udm_cache - admin is running\n");
        return 1;
    }
    if (init_spdk()) {
        return 1;
    }
    /* build device */
    get_device_info(&shared_cache->cache_dev.block_size, &shared_cache->cache_dev.device_size);
    shared_cache->cache_dev.cache_block_num = CACHE_BLOCK_NUMBER;
    shared_cache->cache_dev.block_per_cblock_shift =
        LOG2(CACHE_BLOCK_SIZE / shared_cache->cache_dev.block_size);
    shared_cache->cache_dev.block_per_page_shift =
        LOG2(PAGE_SIZE / shared_cache->cache_dev.block_size);
    unsigned long target_size = (unsigned long)CACHE_BLOCK_NUMBER * CACHE_BLOCK_SIZE;
    if (shared_cache->cache_dev.device_size < target_size) {
        printf("Error: Cache device size is not enough for setting cblock number\n");
        return 1;
    }

    if (init_mapping(&shared_cache->cache_map, shared_cache->cache_dev.block_size,
                     shared_cache->cache_dev.cache_block_num)) {
        return 1;
    }
    wakeup_mg_worker();
    // wakeup_wb_worker();
    spinlock_init(&shared_cache->cache_state.lock);
    shared_cache->cache_state.running = true;
    shared_cache->cache_state.count = 0;
    return 0;
}

int link_ssd_cache(void) {
    shared_cache = link_shm(SHM_CACHE_NAME, sizeof(struct cache));
    if (!shared_cache) {
        return 1;
    }
    if (!shared_cache->cache_state.running) {
        printf("Error: link_udm_cache - shared cache uninitialized\n");
        return 1;
    }
    if (init_spdk()) {
        return 1;
    }
    spinlock_lock(&shared_cache->cache_state.lock);
    shared_cache->cache_state.count++;
    spinlock_unlock(&shared_cache->cache_state.lock);
    return 0;
}

int unlink_ssd_cache(void) {
    if (!shared_cache) {
        printf("Error: unlink_udm_cache - shared cache uninitialized\n");
        return 1;
    }
    if (!shared_cache->cache_state.running) {
        printf("Error: unlink_udm_cache - shared cache uninitialized\n");
        return 1;
    }
    exit_spdk();
    spinlock_lock(&shared_cache->cache_state.lock);
    shared_cache->cache_state.count--;
    spinlock_unlock(&shared_cache->cache_state.lock);

    if (unmap_shm(shared_cache, sizeof(struct cache))) {
        return 1;
    }
    shared_cache = NULL;
    return 0;
}

int exit_ssd_cache(void) {
    if (!shared_cache) {
        printf("Error: exit_udm_cache - shared cache uninitialized\n");
        return 1;
    }
    if (!shared_cache->cache_state.running) {
        printf("Error: exit_udm_cache - shared cache uninitialized\n");
        return 1;
    }
    while (true) {
        spinlock_lock(&shared_cache->cache_state.lock);
        if (shared_cache->cache_state.count <= 0) {
            spinlock_unlock(&shared_cache->cache_state.lock);
            break;
        }
        printf("Waiting for %d user(s) ... \n", shared_cache->cache_state.count);
        spinlock_unlock(&shared_cache->cache_state.lock);
        sleep(1);
    }
    // shutdown_wb_worker();
    shutdown_mg_worker();
    exit_spdk();
    if (unmap_shm(shared_cache, sizeof(struct cache))) {
        return 1;
    }
    if (unlink_shm(SHM_CACHE_NAME)) {
        return 1;
    }
    shared_cache = NULL;
    return 0;
}

void force_exit_ssd_cache(void) {
    // shutdown_wb_worker();
    shutdown_mg_worker();
    unlink_shm(SHM_CACHE_NAME);
}

void info_ssd_cache(void) {
    if (!shared_cache) {
        printf("Error: info_udm_cache - shared cache uninitialized\n");
        return;
    }
    if (!shared_cache->cache_state.running) {
        printf("Error: info_udm_cache - shared cache uninitialized\n");
        return;
    }
    printf("---> Information of cache device <---\n");
    printf("/ block size = %u Bytes\n", shared_cache->cache_dev.block_size);
    printf("/ device size = %lu GB\n", shared_cache->cache_dev.device_size / 1000000000);
    printf("/ cache block num = %u\n", shared_cache->cache_dev.cache_block_num);
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