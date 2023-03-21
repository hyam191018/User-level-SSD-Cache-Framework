#ifndef CACHE_TYPE_H
#define CACHE_TYPE_H

/*
 *	Author: Hyam
 *	Date: 2023/03/17
 *	Description: udm-cache核心資料結構, cache 與 mapping
 */

#include "atomic.h"
#include "config.h"
#include "stdinc.h"
#include "work_queue.h"

struct entry {
	unsigned hash_next;
	unsigned prev;
	unsigned next;
	unsigned short param:2;	// dirty 1, pending_work 1 
	char full_path_name[MAX_PATH_SIZE + 1]; // '\0'
	unsigned cache_page_index;
};

struct entry_space {
	struct entry *begin;
	struct entry *end;
};

struct ilist {
	unsigned nr_elts;	
	unsigned head, tail;
};

struct entry_alloc {
	struct entry_space *es;
	unsigned begin;

	unsigned nr_allocated;
	struct ilist free;
};

struct hash_table {
	struct entry_space *es;
	unsigned long long hash_bits;
	unsigned *buckets;
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

	unsigned hit_time;
	unsigned miss_time;
	unsigned promotion_time;
	unsigned demotion_time;
	unsigned writeback_time;

	work_queue wq;
} mapping;

typedef struct {
	char* bdev_name;
	unsigned block_size;			// default 512 bytes
	unsigned device_size;			// block number
	unsigned cache_block_num;		// cache block (32KB)
	unsigned blocks_per_page;		// 4KB / 512 = 8
	unsigned blocks_per_cache_block;	// 32KB / 512 = 64
} device;

typedef struct {
	bool running;
	unsigned count;			// users and admin number
} state;

struct cache {
	state cache_state;		// for share cache
	device cache_dev;		// SSD info
	mapping cache_map;		// mapping table

	pthread_t mg_worker;	// deal work from work queue
	pthread_t wb_worker;	// rise writeback period
};


#endif
