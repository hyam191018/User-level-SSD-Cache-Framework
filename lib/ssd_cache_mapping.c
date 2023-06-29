#include "config.h"
#include "mapping.h"
#include "shm.h"
#include "spdk.h"
#include "stdinc.h"
#include "work_queue.h"

/* --------------------------------------------------- */

#define INDEXER_NULL ((1u << 28u) - 1u)

static int alloc_es(struct entry_space *es) {
    es->begin = 0;
    es->end = CACHE_BLOCK_NUMBER - 1;
    return 0;
}

static struct entry *__get_entry(struct entry_space *es, unsigned block) {
    struct entry *e = &es->entrys[block];
    return e;
}

static unsigned to_index(struct entry_space *es, struct entry *e) { return e - (&es->entrys[0]); }

static struct entry *to_entry(struct entry_space *es, unsigned block) {
    if (block == INDEXER_NULL) {
        return NULL;
    }
    return __get_entry(es, block);
}

/* --------------------------------------------------- */

static void l_init(struct ilist *l) {
    l->nr_elts = 0;
    l->head = l->tail = INDEXER_NULL;
}

static struct entry *l_head(struct entry_space *es, struct ilist *l) {
    return to_entry(es, l->head);
}

static struct entry *l_tail(struct entry_space *es, struct ilist *l) {
    return to_entry(es, l->tail);
}

static struct entry *l_next(struct entry_space *es, struct entry *e) {
    return to_entry(es, e->next);
}

static struct entry *l_prev(struct entry_space *es, struct entry *e) {
    return to_entry(es, e->prev);
}

static int l_empty(struct ilist *l) { return l->head == INDEXER_NULL; }

static void l_add_head(struct entry_space *es, struct ilist *l, struct entry *e) {
    struct entry *head = l_head(es, l);

    e->next = l->head;
    e->prev = INDEXER_NULL;
    if (head) {
        head->prev = l->head = to_index(es, e);
    } else {
        l->head = l->tail = to_index(es, e);
    }
    l->nr_elts++;
}

static void l_add_tail(struct entry_space *es, struct ilist *l, struct entry *e) {
    struct entry *tail = l_tail(es, l);

    e->next = INDEXER_NULL;
    e->prev = l->tail;

    if (tail) {
        tail->next = l->tail = to_index(es, e);
    } else {
        l->head = l->tail = to_index(es, e);
    }
    l->nr_elts++;
}

static void l_del(struct entry_space *es, struct ilist *l, struct entry *e) {
    struct entry *prev = l_prev(es, e);
    struct entry *next = l_next(es, e);

    if (prev) {
        prev->next = e->next;
    } else {
        l->head = e->next;
    }

    if (next) {
        next->prev = e->prev;
    } else {
        l->tail = e->prev;
    }
    l->nr_elts--;
}

static struct entry *l_pop_head(struct entry_space *es, struct ilist *l) {
    struct entry *e;
    for (e = l_head(es, l); e; e = l_next(es, e)) {
        l_del(es, l, e);
        return e;
    }
    return NULL;
}

/* --------------------------------------------------- */

static void init_allocator(struct entry_alloc *ea, struct entry_space *es, unsigned begin,
                           unsigned end) {
    unsigned i;

    ea->nr_allocated = 0;
    ea->begin = begin;

    l_init(&ea->free);
    for (i = begin; i != end; i++) {
        l_add_tail(es, &ea->free, __get_entry(es, i));
    }
}

static void init_entry(struct entry *e) {
    e->hash_next = INDEXER_NULL;
    e->prev = INDEXER_NULL;
    e->next = INDEXER_NULL;
    e->param = 0;
    strcpy(e->full_path_name, "");
    e->cache_page_index = INDEXER_NULL;
}

static void entry_set_pending(struct entry *e, bool pending) {
    if (pending) {
        e->param |= 1;
    } else {
        e->param &= ~1;
    }
}

static bool entry_get_pending(struct entry *e) {
    if (!e) {
        printf("MSG: Get pending fail, entry is null\n");
        return false;
    }
    return e->param & 1;
}

static void entry_set_dirty(struct entry *e, bool dirty) {
    if (dirty) {
        e->param |= (1 << 1);
    } else {
        e->param &= ~(1 << 1);
    }
}

static bool entry_get_dirty(struct entry *e) {
    if (!e) {
        printf("MSG: Get dirty fail, entry is null\n");
        return false;
    }
    unsigned n = e->param;
    return (n & (1 << 1)) != 0;
}

static void entry_set_alloc(struct entry *e, bool alloc) {
    if (alloc) {
        e->param |= (1 << 2);
    } else {
        e->param &= ~(1 << 2);
    }
}

static bool entry_get_alloc(struct entry *e) {
    if (!e) {
        printf("MSG: Get alloc fail, entry is null\n");
        return false;
    }
    unsigned n = e->param;
    return (n & (1 << 2)) != 0;
}

static struct entry *alloc_entry(struct entry_alloc *ea, struct entry_space *es) {
    struct entry *e;
    if (l_empty(&ea->free)) {
        return NULL;
    }
    e = l_pop_head(es, &ea->free);
    init_entry(e);
    entry_set_alloc(e, true);
    ea->nr_allocated++;

    return e;
}

static void free_entry(struct entry_alloc *ea, struct entry_space *es, struct entry *e) {
    ea->nr_allocated--;
    entry_set_alloc(e, false);
    l_add_tail(es, &ea->free, e);
}

static int allocator_empty(struct entry_alloc *ea) { return l_empty(&ea->free); }

static unsigned get_index(struct entry_alloc *ea, struct entry_space *es, struct entry *e) {
    return to_index(es, e) - ea->begin;
}

static unsigned infer_cblock(mapping *mapping, struct entry *e) {
    return get_index(&mapping->ea, &mapping->es, e);
}

/* --------------------------------------------------- */

/*  djb2 hash */
static unsigned hash_32(char *full_path_name, unsigned cache_page_index,
                        unsigned long long hash_bits) {
    unsigned hash_val = 5381;
    unsigned len = strlen(full_path_name);

    // hash the integer first
    hash_val = ((hash_val << 5) + hash_val) + cache_page_index;

    // hash the string next
    for (unsigned i = 0; i < len; i++) {
        hash_val = ((hash_val << 5) + hash_val) + full_path_name[i]; /* hash * 33 + c */
    }

    hash_val = (hash_val << (32 - hash_bits)) >> (32 - hash_bits);
    return hash_val;
}

static void alloc_hash_table(struct hash_table *ht) {
    unsigned i;
    ht->hash_bits = 31 - __builtin_clz(BUCKETS_NUMBER);
    for (i = 0; i < BUCKETS_NUMBER; i++) {
        ht->buckets[i] = INDEXER_NULL;
    }
}

static struct entry *h_head(struct hash_table *ht, struct entry_space *es, unsigned bucket) {
    return to_entry(es, ht->buckets[bucket]);
}

static struct entry *h_next(struct hash_table *ht, struct entry_space *es, struct entry *e) {
    return to_entry(es, e->hash_next);
}

static void __h_insert(struct hash_table *ht, struct entry_space *es, unsigned bucket,
                       struct entry *e) {
    e->hash_next = ht->buckets[bucket];
    ht->buckets[bucket] = to_index(es, e);
}

static void h_insert(struct hash_table *ht, struct entry_space *es, struct entry *e) {
    unsigned h = hash_32(e->full_path_name, e->cache_page_index, ht->hash_bits);
    __h_insert(ht, es, h, e);
}

static struct entry *__h_lookup(struct hash_table *ht, struct entry_space *es, unsigned h,
                                char *full_path_name, unsigned cache_page_index,
                                struct entry **prev) {
    struct entry *e;
    *prev = NULL;
    for (e = h_head(ht, es, h); e; e = h_next(ht, es, e)) {
        if ((cache_page_index == e->cache_page_index) &&
            (strcmp(full_path_name, e->full_path_name) == 0)) {
            return e;
        }
        *prev = e;
    }

    return NULL;
}

static void __h_unlink(struct hash_table *ht, unsigned h, struct entry *e, struct entry *prev) {
    if (prev) {
        prev->hash_next = e->hash_next;
    } else {
        ht->buckets[h] = e->hash_next;
    }
}

static struct entry *h_lookup(struct hash_table *ht, struct entry_space *es, char *full_path_name,
                              unsigned cache_page_index) {
    struct entry *e, *prev;
    unsigned h = hash_32(full_path_name, cache_page_index, ht->hash_bits);

    e = __h_lookup(ht, es, h, full_path_name, cache_page_index, &prev);
    if (e && prev) {
        /*
         * Move to the front because this entry is likely
         * to be hit again.
         */
        __h_unlink(ht, h, e, prev);
        __h_insert(ht, es, h, e);
    }

    return e;
}

static void h_remove(struct hash_table *ht, struct entry_space *es, struct entry *e) {
    unsigned h = hash_32(e->full_path_name, e->cache_page_index, ht->hash_bits);
    struct entry *prev;

    /*
     * The down side of using a singly linked list is we have to
     * iterate the bucket to remove an item.
     */
    e = __h_lookup(ht, es, h, e->full_path_name, e->cache_page_index, &prev);
    if (e) {
        __h_unlink(ht, h, e, prev);
    }
}

/* --------------------------------------------------- */

static inline int LOG2(unsigned int n) { return (n > 1) ? (1 + LOG2(n >> 1)) : 0; }

int init_mapping(mapping *mapping, unsigned block_size, unsigned cblock_num) {
    if (!block_size || !cblock_num) {
        return 1;
    }
    mapping->block_size = block_size;
    mapping->cache_block_num = cblock_num;
    mapping->block_per_cblock_shift = LOG2(CACHE_BLOCK_SIZE / mapping->block_size);
    mapping->block_per_page_shift = LOG2(PAGE_SIZE / mapping->block_size);

    alloc_es(&mapping->es);
    init_allocator(&mapping->ea, &mapping->es, 0, mapping->cache_block_num);
    l_init(&mapping->clean);
    l_init(&mapping->dirty);
    alloc_hash_table(&mapping->table);

    mapping->hit_time = 0;
    mapping->miss_time = 0;
    mapping->promotion_time = 0;
    mapping->demotion_time = 0;
    mapping->writeback_time = 0;
    spinlock_init(&mapping->mapping_lock);
    return 0;
}

void info_mapping(mapping *mapping) {
    spinlock_lock(&mapping->mapping_lock);
    printf("---> Information of mapping table <---\n");
    printf("/ free  entry = %u\n", mapping->ea.free.nr_elts);
    printf("/ clean entry = %u\n", mapping->clean.nr_elts);
    printf("/ dirty entry = %u\n", mapping->dirty.nr_elts);
    printf("/ promotion time = %u\n", mapping->promotion_time);
    printf("/ demotion  time = %u\n", mapping->demotion_time);
    printf("/ writeback time = %u\n", mapping->writeback_time);
    double hit_ratio = safe_div(mapping->hit_time, mapping->hit_time + mapping->miss_time) * 100;
    printf("/ hit time = %u, miss time = %u, hit ratio = %.2f%%\n", mapping->hit_time,
           mapping->miss_time, hit_ratio);
    spinlock_unlock(&mapping->mapping_lock);
}

/* --------------------------------------------------- */

static bool promotion_free_to_clean(mapping *mapping, char *full_path_name,
                                    unsigned cache_page_index, unsigned *cblock) {
    spinlock_lock(&mapping->mapping_lock);
    if (allocator_empty(&mapping->ea)) {
        spinlock_unlock(&mapping->mapping_lock);
        return false;
    }

    struct entry *e = alloc_entry(&mapping->ea, &mapping->es);
    entry_set_pending(e, true);
    entry_set_dirty(e, false);
    strcpy(e->full_path_name, full_path_name);
    e->cache_page_index = cache_page_index;
    *cblock = infer_cblock(mapping, e);
    spinlock_unlock(&mapping->mapping_lock);
    return true;
}

static void promotion_complete(mapping *mapping, unsigned *cblock, bool success) {
    spinlock_lock(&mapping->mapping_lock);
    struct entry *e = to_entry(&mapping->es, *cblock);
    entry_set_pending(e, false);

    if (success) {
        mapping->promotion_time++;
        // !h, !q, a -> h, q, a
        h_insert(&mapping->table, &mapping->es, e);
        l_add_tail(&mapping->es, entry_get_dirty(e) ? &mapping->dirty : &mapping->clean, e);
    } else {
        // !h, !q, a -> !h, !q, !a
        free_entry(&mapping->ea, &mapping->es, e);
    }
    spinlock_unlock(&mapping->mapping_lock);
}

static bool demotion_clean_to_free(mapping *mapping, unsigned *cblock) {
    spinlock_lock(&mapping->mapping_lock);

    if (mapping->clean.nr_elts <= (mapping->cache_block_num >> 1)) {
        spinlock_unlock(&mapping->mapping_lock);
        return false;
    }
    struct entry *e = l_head(&mapping->es, &mapping->clean);
    l_del(&mapping->es, &mapping->clean, e);
    entry_set_pending(e, true);
    *cblock = infer_cblock(mapping, e);

    spinlock_unlock(&mapping->mapping_lock);
    return true;
}

static void demotion_complete(mapping *mapping, unsigned *cblock, bool success) {
    spinlock_lock(&mapping->mapping_lock);
    struct entry *e = to_entry(&mapping->es, *cblock);
    entry_set_pending(e, false);

    if (success) {
        // h, !q, a -> !h, !q, !a
        mapping->demotion_time++;
        h_remove(&mapping->table, &mapping->es, e);
        free_entry(&mapping->ea, &mapping->es, e);
    } else {
        // h, !q, a -> h, q, a
        l_add_tail(&mapping->es, entry_get_dirty(e) ? &mapping->dirty : &mapping->clean, e);
    }

    spinlock_unlock(&mapping->mapping_lock);
}

static bool writeback_dirty_to_clean(mapping *mapping, unsigned *cblock) {
    spinlock_lock(&mapping->mapping_lock);

    if (!mapping->dirty.nr_elts) {
        spinlock_unlock(&mapping->mapping_lock);
        return false;
    }

    struct entry *e = l_head(&mapping->es, &mapping->dirty);
    // 是否正在搬移
    if (entry_get_pending(e)) {
        spinlock_unlock(&mapping->mapping_lock);
        return false;
    }
    entry_set_pending(e, true);
    entry_set_dirty(e, false);
    l_del(&mapping->es, &mapping->dirty, e);
    *cblock = infer_cblock(mapping, e);

    spinlock_unlock(&mapping->mapping_lock);
    return true;
}

static void writeback_complete(mapping *mapping, unsigned *cblock, bool success) {
    spinlock_lock(&mapping->mapping_lock);
    struct entry *e = to_entry(&mapping->es, *cblock);
    entry_set_pending(e, false);

    if (success) {
        // h, !q, a -> h, q, a
        mapping->writeback_time++;
        l_add_tail(&mapping->es, entry_get_dirty(e) ? &mapping->dirty : &mapping->clean, e);
    } else {
        // h, !q, a -> h, q, a
        l_add_head(&mapping->es, entry_get_dirty(e) ? &mapping->dirty : &mapping->clean, e);
    }
    spinlock_unlock(&mapping->mapping_lock);
}

/* --------------------------------------------------- */

static bool mg_start(mapping *mapping, void *dma_buf, char *full_path_name,
                     unsigned cache_page_index, unsigned cblock, mg_type type) {
    int fd;
    switch (type) {
        case PROMOTION:
            // read from HDD
            fd = open(full_path_name, O_RDONLY | O_DIRECT, 0644);
            if (fd < 0) {
                printf("Error: open file fail\n");
                return false;
            }
            if (pread(fd, dma_buf, CACHE_BLOCK_SIZE, cache_page_index << CBLOCK_SHIFT) < 0) {
                printf("Error: read HDD fail\n");
                return false;
            }
            close(fd);
            // write to SSD
            if (write_spdk(dma_buf, cblock << mapping->block_per_cblock_shift,
                           1 << mapping->block_per_cblock_shift, MG_QUEUE)) {
                printf("Error: write SSD fail\n");
                return false;
            }
            break;
        case DEMOTION:
            // trim SSD
            trim_spdk(cache_page_index << mapping->block_per_cblock_shift,
                      1 << mapping->block_per_cblock_shift, MG_QUEUE);
            break;
        case WRITEBACK:
            // read from SSD
            if (read_spdk(dma_buf, cblock << mapping->block_per_cblock_shift,
                          1 << mapping->block_per_cblock_shift, MG_QUEUE)) {
                printf("Error: read SSD fail\n");
                return false;
            }
            // write to HDD
            fd = open(full_path_name, O_WRONLY | O_DIRECT, 0644);
            if (fd < 0) {
                printf("Error: open file fail\n");
                return false;
            }
            if (pwrite(fd, dma_buf, CACHE_BLOCK_SIZE, cache_page_index << CBLOCK_SHIFT) < 0) {
                printf("Error: write HDD fail\n");
                return false;
            }
            close(fd);
            break;

        default:
            printf("Error: unknow type\n");
            return false;
            break;
    }
    return true;
}

bool do_migration_work(mapping *mapping, void *dma_buf) {
    char full_path_name[MAX_PATH_SIZE];
    unsigned cache_page_index;
    unsigned cblock;
    bool success = true;
    /* get a work */
    if (peak_work(&mapping->wq, full_path_name, &cache_page_index)) {
        if (promotion_free_to_clean(mapping, full_path_name, cache_page_index, &cblock)) {
            success =
                mg_start(mapping, dma_buf, full_path_name, cache_page_index, cblock, PROMOTION);
            // Complete promotion
            promotion_complete(mapping, &cblock, success);
        } else if (demotion_clean_to_free(mapping, &cblock)) {
            success = mg_start(mapping, NULL, NULL, 0, cblock, DEMOTION);
            // Complete demotion
            demotion_complete(mapping, &cblock, success);
        } else if (writeback_dirty_to_clean(mapping, &cblock)) {
            struct entry *e = to_entry(&mapping->es, cblock);
            success = mg_start(mapping, dma_buf, e->full_path_name, e->cache_page_index, cblock,
                               WRITEBACK);
            // Complete writeback
            writeback_complete(mapping, &cblock, success);
        }
        remove_work(&mapping->wq);
        return true;
    }
    return false;
}
/*
bool do_writeback_work(mapping *mapping, void *dma_buf) {
    unsigned cblock;
    bool success = true;
    if (writeback_dirty_to_clean(mapping, &cblock)) {
        struct entry *e = to_entry(&mapping->es, cblock);
        success =
            mg_start(mapping, dma_buf, e->full_path_name, e->cache_page_index, cblock, WRITEBACK);
        //        Complete writeback
        writeback_complete(mapping, &cblock, success);
        return true;
    }
    return false;
}
*/
/* --------------------------------------------------- */

#define to_cache_page_index(page_index) (page_index >> 3)

bool lookup_mapping(mapping *mapping, char *full_path_name, unsigned page_index, unsigned *cblock) {
    spinlock_lock(&mapping->mapping_lock);

    struct entry *e =
        h_lookup(&mapping->table, &mapping->es, full_path_name, to_cache_page_index(page_index));
    if (e) {
        mapping->hit_time++;
        // 判斷是否在clean queue，將其移動到clean queue mru端
        if (!entry_get_pending(e) && !entry_get_dirty(e)) {
            l_del(&mapping->es, entry_get_dirty(e) ? &mapping->dirty : &mapping->clean, e);
            l_add_tail(&mapping->es, entry_get_dirty(e) ? &mapping->dirty : &mapping->clean, e);
        }
        *cblock = infer_cblock(mapping, e);
    } else {
        mapping->miss_time++;
        // 告知udm-cache，將其搬移到SSD
        insert_work(&mapping->wq, full_path_name, to_cache_page_index(page_index));
    }
    spinlock_unlock(&mapping->mapping_lock);
    return (e != NULL);
}

bool lookup_mapping_with_insert(mapping *mapping, char *full_path_name, unsigned page_index,
                                unsigned *cblock) {
    spinlock_lock(&mapping->mapping_lock);

    struct entry *e =
        h_lookup(&mapping->table, &mapping->es, full_path_name, to_cache_page_index(page_index));
    if (e) {
        mapping->hit_time++;
        // 判斷是否在clean queue，將其移動到clean queue mru端
        if (!entry_get_pending(e) && !entry_get_dirty(e)) {
            l_del(&mapping->es, entry_get_dirty(e) ? &mapping->dirty : &mapping->clean, e);
            l_add_tail(&mapping->es, entry_get_dirty(e) ? &mapping->dirty : &mapping->clean, e);
        }
        *cblock = infer_cblock(mapping, e);
    } else {
        mapping->miss_time++;
        // 若沒有在work queue，且free entry還有
        if (contains_work(&mapping->wq, full_path_name, to_cache_page_index(page_index))) {
            spinlock_unlock(&mapping->mapping_lock);
            return NULL;
        }
        if (allocator_empty(&mapping->ea)) {
            // 告知udm-cache，將其搬移到SSD
            insert_work(&mapping->wq, full_path_name, to_cache_page_index(page_index));
            spinlock_unlock(&mapping->mapping_lock);
            return NULL;
        }
        e = alloc_entry(&mapping->ea, &mapping->es);
        entry_set_dirty(e, true);
        strcpy(e->full_path_name, full_path_name);
        e->cache_page_index = to_cache_page_index(page_index);

        h_insert(&mapping->table, &mapping->es, e);
        l_add_tail(&mapping->es, entry_get_dirty(e) ? &mapping->dirty : &mapping->clean, e);
        *cblock = infer_cblock(mapping, e);
    }
    spinlock_unlock(&mapping->mapping_lock);
    return (e != NULL);
}

bool remove_mapping(mapping *mapping, char *full_path_name, unsigned page_index) {
    spinlock_lock(&mapping->mapping_lock);

    struct entry *e =
        h_lookup(&mapping->table, &mapping->es, full_path_name, to_cache_page_index(page_index));
    if (e) {
        l_del(&mapping->es, entry_get_dirty(e) ? &mapping->dirty : &mapping->clean, e);
        h_remove(&mapping->table, &mapping->es, e);
        free_entry(&mapping->ea, &mapping->es, e);
    }
    spinlock_unlock(&mapping->mapping_lock);
    return (e != NULL);
}

void set_dirty_after_write(mapping *mapping, unsigned *cblock, bool dirty) {
    spinlock_lock(&mapping->mapping_lock);

    struct entry *e = to_entry(&mapping->es, *cblock);
    // 有極低的機率 write cache時，該cblock被demotion掉，故需確認他還存在
    if (!entry_get_alloc(e)) {
        spinlock_unlock(&mapping->mapping_lock);
        return;
    }
    if (entry_get_pending(e)) {
        entry_set_dirty(e, true);
    } else {
        l_del(&mapping->es, entry_get_dirty(e) ? &mapping->dirty : &mapping->clean, e);
        entry_set_dirty(e, true);
        l_add_tail(&mapping->es, entry_get_dirty(e) ? &mapping->dirty : &mapping->clean, e);
    }
    spinlock_unlock(&mapping->mapping_lock);
}