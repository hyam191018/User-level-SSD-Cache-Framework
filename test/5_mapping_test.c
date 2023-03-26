#include <pthread.h> /* pthread */
#include <signal.h>  /* signal*/
#include <stdio.h>   /* printf */
#include <stdlib.h>  /* malloc */
#include <time.h>    /* random */

#include "cache_type.h"
#include "mapping.h"

/*     實驗記錄 r3 w3 optw4 跑100萬次
 *
 * ---> Information of mapping table <---
 * / free  entrys = 0
 * / clean entrys = 1024
 * / dirty entrys = 0
 * / promotion time = 1475686
 * / demotion  time = 1525890
 * / writeback time = 3601398
 * / hit time = 4997965, miss time = 5002035, hit ratio = 50%
 * free rc = 0
 * exit rc = 0
 * 282.48user
 * 107.89system
 * 2:51.35elapsed
 * 227%CPU (0avgtext+0avgdata 6904maxresident)k
 * 0inputs+0outputs (0major+613minor)pagefaults 0swaps
 */

#define do_printf false
#define read_users 3      // read hit, read miss, write miss (no optimizable) 的使用者數量
#define write_users 3     // write hit 的使用者數量
#define optw_users 4      // write miss but optimizable 的使用者數量
#define test_time 100000  // 測試次數，一次請求等同4~32KB IO
#define EXCEPT 50         // 期望的 hit ratio
#define to_cache_page_index(page_index) \
    (page_index >> 3)  // page_index為4KB，cache_page_index為32KB

// 可能會影響效能
// 定義一個互斥鎖(讓mg&wb worker可以一直運作，直到users都結束)
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
// 當count==user數，mg&wb worker結束
int count = 0;

// 期望的 hit ratio = 裝置大小 / 檔案大小
// 假設期望 hit ration 50%, 裝置有100G, 那檔案應該要有200G
const int MAX_PAGE_INDEX = CACHE_BLOCK_NUMBER * CACHE_BLOCK_SIZE / 1024 * 100 / EXCEPT / 4;

mapping mp;

// 模擬 read hit, read miss, write miss (no optimizable)
static void *read_func(void *arg) {
    for (int i = 0; i < test_time; i++) {
        char *name = "test";
        unsigned page_index = rand() % MAX_PAGE_INDEX;
        unsigned cblock;
        if (lookup_mapping(&mp, name, page_index, &cblock)) {
            if (do_printf)
                printf("read cache\n");
        } else {
            if (do_printf)
                printf("read origin\n");
        }
    }

    pthread_mutex_lock(&lock);
    count++;
    pthread_mutex_unlock(&lock);

    return NULL;
}

// 模擬 write hit，並call set_dirty_after_write
static void *write_func(void *arg) {
    for (int i = 0; i < test_time; i++) {
        char *name = "test";
        unsigned page_index = rand() % MAX_PAGE_INDEX;
        unsigned cblock;
        if (lookup_mapping(&mp, name, page_index, &cblock)) {
            if (do_printf)
                printf("write cache\n");
            set_dirty_after_write(&mp, &cblock, true);
        } else {
            if (do_printf)
                printf("write origin\n");
        }
    }

    pthread_mutex_lock(&lock);
    count++;
    pthread_mutex_unlock(&lock);

    return NULL;
}

// 模擬 write miss but optimizable
static void *optw_func(void *arg) {
    for (int i = 0; i < test_time; i++) {
        char *name = "test";
        unsigned page_index = rand() % MAX_PAGE_INDEX;
        unsigned cblock;
        if (lookup_mapping_with_insert(&mp, name, page_index, &cblock)) {
            if (do_printf)
                printf("optimizable write cache\n");
            set_dirty_after_write(&mp, &cblock, true);
        } else {
            if (do_printf)
                printf("write origin\n");
        }
    }

    pthread_mutex_lock(&lock);
    count++;
    pthread_mutex_unlock(&lock);

    return NULL;
}

// admin會開兩個thread 執行 migration與writeback
static void *mg_worker_func(void *arg) {
    while (true) {
        do_migration_work(&mp);
        pthread_mutex_lock(&lock);
        // 檢查別人完成了沒
        if (count == (read_users + optw_users + write_users)) {
            pthread_mutex_unlock(&lock);
            break;
        }
        pthread_mutex_unlock(&lock);
    }

    return NULL;
}

static void *wb_worker_func(void *arg) {
    while (true) {
        do_writeback_work(&mp);
        pthread_mutex_lock(&lock);
        // 檢查別人完成了沒
        if (count == (read_users + optw_users + write_users)) {
            pthread_mutex_unlock(&lock);
            break;
        }
        pthread_mutex_unlock(&lock);
    };

    return NULL;
}

int main(void) {
    srand(time(NULL));

    printf("init rc = %d\n", init_mapping(&mp, 512, CACHE_BLOCK_NUMBER));

    pthread_t read_user[read_users];
    pthread_t optw_user[optw_users];
    pthread_t write_user[write_users];
    pthread_t mg_worker;
    pthread_t wb_worker;

    for (int i = 0; i < read_users; i++) {
        pthread_create(&read_user[i], NULL, read_func, NULL);
    }

    for (int i = 0; i < write_users; i++) {
        pthread_create(&write_user[i], NULL, write_func, NULL);
    }
    for (int i = 0; i < optw_users; i++) {
        pthread_create(&optw_user[i], NULL, optw_func, NULL);
    }

    pthread_create(&mg_worker, NULL, mg_worker_func, NULL);
    pthread_create(&wb_worker, NULL, wb_worker_func, NULL);

    for (int i = 0; i < read_users; i++) {
        pthread_join(read_user[i], NULL);
    }

    for (int i = 0; i < write_users; i++) {
        pthread_join(write_user[i], NULL);
    }
    for (int i = 0; i < optw_users; i++) {
        pthread_join(optw_user[i], NULL);
    }

    pthread_join(mg_worker, NULL);
    pthread_join(wb_worker, NULL);
    info_mapping(&mp);
    printf("free rc = %d\n", free_mapping(&mp));
    printf("exit rc = %d\n", exit_mapping());
}