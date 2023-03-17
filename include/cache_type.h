#ifndef CACHE_TYPE_H
#define CACHE_TYPE_H

/*
 *	Author: Hyam
 *	Date: 2023/03/17
 *	Description: udm-cache核心資料結構, cache與mapping
 */

struct entry {
	unsigned hash_next;
	unsigned prev;
	unsigned next;

	/* dirty 1, allocated 1, pending_work 1 */
	unsigned short param;	

	char* full_path_name;
	unsigned cache_page;
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
	unsigned block_size;
	unsigned cblock_num;

	struct entry_space es;
	struct entry_alloc cache_alloc;
	struct ilist clean;
	struct ilist dirty;
	struct hash_table table;

	unsigned hit_time;
	unsigned miss_time;
} mapping;

typedef struct {
	char* bdev_name;
	unsigned block_size;			/* default 512 bytes */
	unsigned device_size;			/* block number */
	unsigned cache_block_num;		/* cache block (32KB) */
	unsigned blocks_per_page;		/* 4KB / 512 = 8 */
	unsigned blocks_per_cache_block;	/* 32KB / 512 = 64 */
} device;

struct cache {
	device cache_dev;		/* SSD info */
	mapping cache_map;		/* mapping table */
};


#endif
