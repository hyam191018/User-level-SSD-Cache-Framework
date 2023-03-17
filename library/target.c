#include "target.h"
#include "stdinc.h"


/* --------------------------------------------------- */

static int map_to_cache(struct cache* cache, struct pio* pio, unsigned cblock){
    printf("MSG: map to cache\n");
    return 0;
}

static int map_to_origin(struct cache* cache, struct pio* pio){
    printf("MSG: map to origin\n");
    return 0;
}

static int check_pio(struct pio* pio){
    return ((8 - (pio->page_index & 7)) >= pio->pio_cnt) ? true : false;
}

static int overwritable(struct pio* pio){
    /* I think the probability of overwritable is relatively low (for 4KB random rw) */
    return unlikely(pio->pio_cnt == 8 && (pio->page_index & 0b111) == 0) ? true : false;
}

static int optimizable(struct pio* pio){
    return (pio->operation == WRITE) && overwritable(pio);
}

static int map_pio(struct cache* cache, struct pio* pio){

	if(unlikely(!check_pio(pio))){
        printf("MSG: check pio fail\n");
        return -1;
    }

    unsigned cblock = 0;
    int hit = false; // lookup_mapping(cache->cache_map, pio->full_path_name, pio->page_index, &cblock);

    if(hit){
        return map_to_cache(cache, pio, cblock);
    }else if(optimizable(pio)){
        printf("MSG: optimizable\n");
        int res = false; // insert_mapping(cache->cache_map, pio->full_path_name, pio->page_index, &cblock);
        if(res){
            return map_to_cache(cache, pio, cblock);
        }else{
            /* SSD is full */
            return map_to_origin(cache, pio);
        }
    }else{
        return map_to_origin(cache, pio);
    }

    return -1;
}

int _submit_pio(struct cache* cache, struct pio* pio){
    if(pio->operation == DISCARD) {
        return -1;
    }

    return map_pio(cache, pio);
}