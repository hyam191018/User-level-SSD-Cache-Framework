#ifndef CONFIG_H
#define CONFIG_H

/*
 *	Author: Hyam
 *	Date: 2023/03/17
 *	Description: 對整個udm-cache架構的變數定義集合
 */

typedef enum { READ, WRITE, DISCARD } operate;
typedef enum { false, true } bool;

/* udm-cache setting */
#define CACHE_BLOCK_SIZE (1<<15)
#define PAGE_SIZE (1<<12)

/* share memory */
#define SHM_CACHE_NAME "/udm_cache"
#define SHM_BDEV_NAME_SIZE 10
#define SHM_BDEV_NAME "/bdev_name"
#define SHM_ENTRY_SPACE "/es"
#define SHM_BUCKETS "/buckets"

/* spdk setting */
#define BDEV_NAME "Nvme0n1"
#define JSON_CONFIG "bdev.json"

/* may increase hit ratio */
#define unlikely(x) __builtin_expect(!!(x), 0)

/* instead of a/b*/
#define safe_div(a, b) ((b != 0) ? (a / b) : 0)

#endif
