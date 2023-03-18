#include "target.h"
#include "mapping.h"
#include "stdinc.h"

static int map_to_cache(struct cache* cache, struct pio* pio, unsigned cblock) {
    // printf("MSG: map to cache\n");
    return 0;
}

static int map_to_origin(struct cache* cache, struct pio* pio) {
    // printf("MSG: map to origin\n");
    return 0;
}

static int check_pio(struct pio* pio) {
    return (8 - (pio->page_index & 7)) >= pio->pio_cnt;
}

static int overwritable(struct pio* pio) {
    // The probability of overwritable is relatively low for 4KB random read/write
    return pio->pio_cnt == 8 && (pio->page_index & 0b111) == 0;
}

static int optimizable(struct pio* pio) {
    return pio->operation == WRITE && overwritable(pio);
}

static int map_pio(struct cache* cache, struct pio* pio) {
    int rc = 0;
    if (!check_pio(pio)) {
        rc = 1;
        goto err;
    }

    unsigned cblock = 0;
    bool hit = lookup_mapping(&cache->cache_map, pio->full_path_name, pio->page_index, &cblock);

    if (hit) {
        rc += map_to_cache(cache, pio, cblock);
    } else if (optimizable(pio)) {
        bool success = insert_mapping_before_io(&cache->cache_map, pio->full_path_name, pio->page_index, &cblock);
        if(success){
            rc += map_to_cache(cache, pio, cblock);
            insert_mapping_after_io(&cache->cache_map, &cblock, rc == 0 ? true : false );
        }else{
            rc += map_to_origin(cache, pio);
        }
    } else {
        rc += map_to_origin(cache, pio);
    }
err:
    return rc;
}

/* --------------------------------------------------- */

int _submit_pio(struct cache* cache, struct pio* pio) {
    if (pio->operation == DISCARD) {
        return 1;
    }

    return map_pio(cache, pio);
}