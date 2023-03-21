#include "mapping.h"
#include "cache_type.h"
#include <stdio.h>   /* printf */
#include <stdlib.h>  /* malloc */
#include <time.h>    /* random */
#include <pthread.h> /* pthread */

#define users 1
#define test_time 1000000
#define EXCEPT 80 // 期望的hit ration
#define to_cache_page_index(page_index) (page_index >> 3)

// 期望的 hit ratio = 裝置大小 / 檔案大小
// 假設期望 hit ration 50%, 裝置有100G, 那檔案應該要有200G
const int MAX_PAGE_INDEX = CACHE_BLOCK_NUMBER * CACHE_BLOCK_SIZE / 1024 * 100 / EXCEPT / 4;

mapping mp;

// 模擬read測試
static void *read_func(void *arg)
{
    for (int i = 0; i < test_time; i++)
    {
        char *name = "test";
        unsigned page_index = rand() % MAX_PAGE_INDEX;
        unsigned cblock;
        lookup_mapping(&mp, name, page_index, &cblock) ? printf("Hit\n") : printf("Miss\n");
    }
    return NULL;
}

// 模擬optimizable write測試
static void *optw_func(void *arg)
{
    for (int i = 0; i < test_time; i++)
    {
        char *name = "test";
        unsigned page_index = rand() % MAX_PAGE_INDEX;
        unsigned cblock;
        lookup_mapping(&mp, name, page_index, &cblock);
    }
    return NULL;
}

// 模擬write hit後，set_dirty_after_write測試
static void *write_func(void *arg)
{
    for (int i = 0; i < test_time; i++)
    {
        char *name = "test";
        unsigned page_index = rand() % MAX_PAGE_INDEX;
        unsigned cblock;
        lookup_mapping(&mp, name, page_index, &cblock);
    }
    return NULL;
}

// admin會開兩個thread 執行migration與writeback
static void *mg_worker_func(void *arg)
{
    for (int i = 0; i < test_time * users; i++)
    {
        do_migration_work(&mp);
    }
    return NULL;
}

static void *wb_worker_func(void *arg)
{
    for (int i = 0; i < test_time * users; i++)
    {
        do_writeback_work(&mp);
    }
    return NULL;
}

int main(void)
{
    srand(time(NULL));
    printf("init rc = %d\n", init_mapping(&mp, 512, CACHE_BLOCK_NUMBER));

    pthread_t read_user[users];
    pthread_t optw_user[users];
    pthread_t write_user[users];
    pthread_t mg_worker;
    pthread_t wb_worker;

    for (int i = 0; i < users; i++)
    {
        pthread_create(&read_user[i], NULL, read_func, NULL);
    }

    for (int i = 0; i < users; i++)
    {
        pthread_create(&optw_user[i], NULL, optw_func, NULL);
    }

    for (int i = 0; i < users; i++)
    {
        pthread_create(&write_user[i], NULL, write_func, NULL);
    }

    pthread_create(&mg_worker, NULL, mg_worker_func, NULL);
    pthread_create(&wb_worker, NULL, wb_worker_func, NULL);

    for (int i = 0; i < users; i++)
    {
        pthread_join(read_user[i], NULL);
    }

    for (int i = 0; i < users; i++)
    {
        pthread_join(optw_user[i], NULL);
    }

    for (int i = 0; i < users; i++)
    {
        pthread_join(write_user[i], NULL);
    }

    info_mapping(&mp);
    printf("free rc = %d\n", free_mapping(&mp));
    printf("exit rc = %d\n", exit_mapping());
}