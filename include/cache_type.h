#ifndef CACHE_TYPE_H
#define CACHE_TYPE_H

/*
 *  Author: Hyam
 *  Date: 2023/03/17
 *  Description: udm-cache核心資料結構, cache 與 mapping
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
    struct entry* begin;
    struct entry* end;
};

struct ilist {
    unsigned nr_elts;
    unsigned head, tail;
};

struct entry_alloc {
    struct entry_space* es;
    unsigned begin;
    unsigned nr_allocated;
    struct ilist free;
};

struct hash_table {
    struct entry_space* es;
    unsigned long long hash_bits;
    unsigned* buckets;
};

typedef struct {
    spinlock mapping_lock;
    unsigned block_size;
    unsigned cblock_num;

    struct entry_space es;
    struct entry_alloc ea;
    struct ilist clean;
    struct ilist dirty;
    struct hash_table table;

    unsigned long hit_time;
    unsigned long miss_time;
    unsigned long promotion_time;
    unsigned long demotion_time;
    unsigned long writeback_time;
    work_queue wq;
} mapping;

typedef struct {
    char bdev_name[MAX_PATH_SIZE];
    unsigned block_size;              // 通常是 512 Bytes
    unsigned long device_size;        // LBA的數量
    unsigned long cache_block_num;    // cache block的數量 (cache block 32KB)
    unsigned blocks_per_page;         // 4KB / 512 = 8
    unsigned blocks_per_cache_block;  // 32KB / 512 = 64
} device;

typedef struct {
    bool running;    // 是否已經init成功
    unsigned count;  // 使用人數(包含admin)
    spinlock lock;   // protect init, link cache
} state;

struct cache {
    state cache_state;  // share cache 的管理
    device cache_dev;   // SSD的資訊，由SPDK負責
    mapping cache_map;  // 管理hash table, clean, dirty, free queue

    pthread_t mg_worker;  // 週期性的去work queue找work
    pthread_t wb_worker;  // 週期性的發起writeback
};

#endif
