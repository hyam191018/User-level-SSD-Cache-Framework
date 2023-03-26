#include "mapping.h"
#include "stdinc.h"
#include "target.h"

/* --------------------------------------------------- */

static int write_cache(struct cache *cache, struct pio *pio, unsigned cblock) {
    printf("write to cache\n");
    set_dirty_after_write(&cache->cache_map, &cblock, true);
    return 0;
}

static int read_cache(struct cache *cache, struct pio *pio, unsigned cblock) {
    printf("read from cache\n");
    return 0;
}

static int write_origin(struct cache *cache, struct pio *pio) {
    printf("write to origin\n");
    return 0;
}

static int read_origin(struct cache *cache, struct pio *pio) {
    printf("read from origin\n");
    return 0;
}

static int map_to_cache(struct cache *cache, struct pio *pio, unsigned cblock) {
    if (pio->operation == READ) {
        read_cache(cache, pio, cblock);
    } else {
        write_cache(cache, pio, cblock);
    }
    return 0;
}

static int map_to_origin(struct cache *cache, struct pio *pio) {
    if (pio->operation == READ) {
        read_origin(cache, pio);
    } else {
        write_origin(cache, pio);
    }
    return 0;
}

/* --------------------------------------------------- */

static int check_pio(struct pio *pio) { return (8 - (pio->page_index & 7)) >= pio->pio_cnt; }

static int overwritable(struct pio *pio) {
    // The probability of overwritable is relatively low for 4KB random
    // read/write
    return pio->pio_cnt == 8 && (pio->page_index & 0b111) == 0;
}

static int optimizable(struct pio *pio) { return pio->operation == WRITE && overwritable(pio); }

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
        return 1;
    }
    return map_pio(cache, pio);
}