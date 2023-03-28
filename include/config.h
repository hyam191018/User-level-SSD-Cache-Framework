#ifndef CONFIG_H
#define CONFIG_H

/*
 *  Author: Hyam
 *  Date: 2023/03/17
 *  Description: 對整個udm-cache架構的變數定義集合
 */

typedef enum { READ, WRITE, DISCARD } operate;

/* udm-cache setting */
#define CACHE_BLOCK_SIZE (1 << 15)                // 32KB
#define CACHE_BLOCK_NUMBER (1 << 15)              // 32K*32KB=1GB(SSD)
#define BUCKETS_NUMBER (CACHE_BLOCK_NUMBER >> 1)  // roundup_pow_of_two
#define PAGE_SIZE (1 << 12)                       // 4KB
#define BLOCKS_PER_PAGE (1 << 3)                  // 4KB / 512 = 8
#define BLOCKS_PER_CACHE_BLOCK (1 << 6)           // 32KB / 512 = 64
#define MAX_PATH_SIZE (1 << 5)                    // full_path_name length
#define WRITEBACK_DELAY 500000000                 // ns (in dm-cache is 500ms)
#define MIGRATION_DELAY 100000000                 // ns (100ms)
#define MAX_WORKQUEUE_SIZE (CACHE_BLOCK_NUMBER >> 1)

/* share memory */
#define SHM_CACHE_NAME "/udm_cache"

/* spdk setting */
#define BDEV_NAME "Nvme0n1"
#define JSON_CONFIG "bdev.json"

/* may increase hit ratio */
#define unlikely(x) __builtin_expect(!!(x), 0)

/* instead of a/b*/
#define safe_div(a, b) ((b != 0) ? (a / b) + !!(a % b) : 0)

#endif
