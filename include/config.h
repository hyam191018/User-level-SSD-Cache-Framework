#ifndef CONFIG_H
#define CONFIG_H

/**
 *  Author: Hyam
 *  Date: 2023/03/17
 *  Description: udm-cache架構的定義集合
 */

typedef enum { READ, WRITE, DISCARD } operate;
typedef enum { PROMOTION, DEMOTION, WRITEBACK } mg_type;

#define CBLOCK_SHIFT 15  // The default cache block size is 32KB
#define CACHE_BLOCK_SIZE (1 << CBLOCK_SHIFT)

#define CACHE_BLOCK_NUMBER (1 << 15)  // 32K x 32KB = 1GB (cache device size)
#define BUCKETS_NUMBER (CACHE_BLOCK_NUMBER)

#define PAGE_SHIFT 12  // The default page size is 4KB
#define PAGE_SIZE (1 << PAGE_SHIFT)
#define MOD_PAGE_PER_CBLOCK_SHIFT 0b111  // and 0b111 = mod 8

#define MAX_PATH_SIZE (1 << 5)                        // full_path_name maximum length
#define WRITEBACK_DELAY 10000000                      // 10ms
#define MIGRATION_DELAY 10000000                      // 10ms
#define MAX_WORKQUEUE_SIZE (CACHE_BLOCK_NUMBER >> 1)  // promotion queue

/* share memory */
#define SHM_CACHE_NAME "/udm_cache"

/* spdk setting */
#define NVME_ADDR "0000:04:00.0"
#define IODEPTH 32

/* may increase hit ratio */
#define unlikely(x) __builtin_expect(!!(x), 0)

/* instead of a/b*/
#define safe_div(a, b) ((b != 0) ? ((double)(a) / (double)(b)) : 0.0)

#endif
