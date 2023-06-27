#ifndef CACHE_H
#define CACHE_H

/**
 *  @author Hyam
 *  @date 2023/03/17
 *  @brief udm-cache核心資料結構, cache 與 mapping
 */

#include "atomic.h"
#include "config.h"
#include "stdinc.h"
#include "work_queue.h"

struct entry {
    unsigned hash_next;
    unsigned prev;
    unsigned next;
    unsigned short param : 3;
    char full_path_name[MAX_PATH_SIZE + 1];  // '\0'
    unsigned cache_page_index;
};

struct entry_space {
    struct entry entrys[CACHE_BLOCK_NUMBER];
    unsigned begin;
    unsigned end;
};

struct ilist {
    unsigned nr_elts;
    unsigned head, tail;
};

struct entry_alloc {
    unsigned begin;
    unsigned nr_allocated;
    struct ilist free;
};

struct hash_table {
    unsigned long long hash_bits;
    unsigned buckets[BUCKETS_NUMBER];
};

typedef struct {
    unsigned block_size;              // 通常是 512 Bytes
    unsigned cache_block_num;         // cache block的數量 (cache block 32KB)
    unsigned block_per_cblock_shift;  // 通常是 32KB / 512 Bytes = 64 = 1 << 6
    unsigned block_per_page_shift;    // 通常是 4KB / 512 Bytes = 8 = 1 << 3

    struct entry_space es;
    struct entry_alloc ea;
    struct ilist clean;
    struct ilist dirty;
    struct hash_table table;

    spinlock mapping_lock;
    unsigned hit_time;
    unsigned miss_time;
    unsigned promotion_time;
    unsigned demotion_time;
    unsigned writeback_time;
    work_queue wq;
} mapping;

typedef struct {
    unsigned block_size;              // 通常是 512 Bytes
    unsigned long device_size;        // LBA的數量
    unsigned cache_block_num;         // cache block的數量 (cache block 32KB)
    unsigned block_per_cblock_shift;  // 通常是 32KB / 512 Bytes = 64 = 1 << 6
    unsigned block_per_page_shift;    // 通常是 4KB / 512 Bytes = 8 = 1 << 3
} device;

typedef struct {
    bool running;    // 是否已經init成功
    unsigned count;  // 使用中的process數(包含admin)
    spinlock lock;   // protect init, link cache
} state;

struct cache {
    state cache_state;    // share cache 的管理
    device cache_dev;     // SSD的資訊，由SPDK負責
    mapping cache_map;    // 管理hash table, clean, dirty, free queue
    pthread_t mg_worker;  // 週期性的去work queue找work
    pthread_t wb_worker;  // 週期性的發起writeback
};

#endif
