#include "mapping.h"
#include "spdk.h"
#include "stdinc.h"
#include "target.h"

/* --------------------------------------------------- */

static void pio_to_iovec(struct pio *pio, struct iovec *iov, int iov_cnt) {
    for (int i = 0; i < iov_cnt; i++) {
        iov[i].iov_base = pio->buffer;
        iov[i].iov_len = strlen(pio->buffer) < PAGE_SIZE ? strlen(pio->buffer) : PAGE_SIZE;
        iov[i].iov_len = PAGE_SIZE;
        pio = pio->next;
    }
}

static int write_cache(struct cache *cache, struct pio *pio, unsigned cblock) {
    unsigned target_block =
        (cblock << cache->cache_map.block_per_cblock_shift) +
        ((pio->page_index & MOD_PAGE_PER_CBLOCK_SHIFT) << cache->cache_dev.block_per_page_shift);
    int rc;
    int iov_cnt = pio->pio_cnt;
    struct iovec *iovs = malloc(sizeof(struct iovec) * iov_cnt);
    pio_to_iovec(pio, iovs, iov_cnt);
    rc = writev_spdk(iovs, iov_cnt, target_block, iov_cnt << cache->cache_dev.block_per_page_shift,
                     IO_QUEUE);
    if (rc) {
        free(iovs);
        return rc;
    }
    set_dirty_after_write(&cache->cache_map, &cblock, true);
    free(iovs);
    return rc;
}

static int read_cache(struct cache *cache, struct pio *pio, unsigned cblock) {
    unsigned target_block =
        (cblock << cache->cache_map.block_per_cblock_shift) +
        ((pio->page_index & MOD_PAGE_PER_CBLOCK_SHIFT) << cache->cache_dev.block_per_page_shift);
    int rc;
    int iov_cnt = pio->pio_cnt;
    struct iovec *iovs = malloc(sizeof(struct iovec) * iov_cnt);
    pio_to_iovec(pio, iovs, iov_cnt);
    rc = readv_spdk(iovs, iov_cnt, target_block, iov_cnt << cache->cache_dev.block_per_page_shift,
                    IO_QUEUE);
    if (rc) {
        free(iovs);
        return rc;
    }
    free(iovs);
    return rc;
}

static int write_origin(struct cache *cache, struct pio *pio) {
    /* pio to iovec */
    int iov_cnt = pio->pio_cnt;
    struct iovec *iov = (struct iovec *)malloc(sizeof(struct iovec) * iov_cnt);
    pio_to_iovec(pio, iov, iov_cnt);

    int res = 0;

    /* open file and direct IO */
    if (!pio->fd) {
        int fd = open(pio->full_path_name, O_WRONLY | O_CREAT | O_DIRECT, 0644);
        if (fd < 0) {
            printf("Error: open file fail\n");
            return 1;
        }
        res = pwritev(fd, iov, iov_cnt, (pio->page_index << 12));
        close(fd);
    } else {
        res = pwritev(pio->fd, iov, iov_cnt, (pio->page_index << 12));
    }

    free(iov);
    return !(res == 0);
}

static int read_origin(struct cache *cache, struct pio *pio) {
    /* pio to iovec */
    int iov_cnt = pio->pio_cnt;
    struct iovec *iov = (struct iovec *)malloc(sizeof(struct iovec) * iov_cnt);
    pio_to_iovec(pio, iov, iov_cnt);

    int res = 0;

    /* open file and direct IO */
    if (!pio->fd) {
        int fd = open(pio->full_path_name, O_RDONLY | O_CREAT | O_DIRECT, 0644);
        if (fd < 0) {
            printf("Error: open file fail\n");
            return 1;
        }
        res = preadv(fd, iov, iov_cnt, (pio->page_index << 12));
        close(fd);
    } else {
        res = preadv(pio->fd, iov, iov_cnt, (pio->page_index << 12));
    }

    free(iov);
    return !(res == 0);
}

static int map_to_cache(struct cache *cache, struct pio *pio, unsigned cblock) {
    if (pio->operation == READ) {
        return read_cache(cache, pio, cblock);
    } else {
        return write_cache(cache, pio, cblock);
    }
    return 1;
}

static int map_to_origin(struct cache *cache, struct pio *pio) {
    if (pio->operation == READ) {
        return read_origin(cache, pio);
    } else {
        return write_origin(cache, pio);
    }
    return 1;
}

/* --------------------------------------------------- */

static int check_pio(struct pio *pio) { return (8 - (pio->page_index & 7)) >= pio->pio_cnt; }

static int overwritable(struct pio *pio) {
    // The probability of overwritable is relatively low for 4KB random read/write
    return pio->pio_cnt == 8 && (pio->page_index & 0b111) == 0;
}

static int optimizable(struct pio *pio) { return pio->operation == WRITE && overwritable(pio); }

static int discard_pio(struct cache *cache, struct pio *pio) {
    if (!check_pio(pio)) {
        return 1;
    }

    return remove_mapping(&cache->cache_map, pio->full_path_name, pio->page_index);
}

static int map_pio(struct cache *cache, struct pio *pio) {
    if (!check_pio(pio)) {
        return 1;
    }

    unsigned cblock;
    bool hit_or_inserted = false;
    if (unlikely(optimizable(pio))) {
        hit_or_inserted = lookup_mapping_with_insert(&cache->cache_map, pio->full_path_name,
                                                     pio->page_index, &cblock);
    } else {
        hit_or_inserted =
            lookup_mapping(&cache->cache_map, pio->full_path_name, pio->page_index, &cblock);
    }
    if (hit_or_inserted) {
        return map_to_cache(cache, pio, cblock);
    }
    return map_to_origin(cache, pio);
}

/* --------------------------------------------------- */

int _submit_pio(struct cache *cache, struct pio *pio) {
    if (pio->operation == DISCARD) {
        return discard_pio(cache, pio);
    }
    return map_pio(cache, pio);
}