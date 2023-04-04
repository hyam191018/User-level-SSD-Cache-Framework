#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "cache_api.h"
#include "spdk.h"
#include "stdinc.h"

/**
 *  Author: Hyam
 *  Date: 2023/03/28
 *  Description: 壓力測試 admin submit pio (read only)
 */

// randwrite 100w  28s no mg
// randread  100w 110s no mg
// randread  100w  135s mg 100ms 1% hit ratio
// randwrite 100w   42s mg 100ms 0.32% hit ratio

#define ROUND 1000000  // submit次數
#define EXCEPT 50      // 期望的 hit ratio
#define FILE_NAME "testfile"
const unsigned long long MAX_PAGE_INDEX =
    (CACHE_BLOCK_NUMBER * CACHE_BLOCK_SIZE * 100ull) / (1024 * EXCEPT * 4ull);

void* buffer;
// 隨機4K讀寫
static void send_pio(void) {
    unsigned page_index = rand() % MAX_PAGE_INDEX;
    operate operation = WRITE;
    if (operation == WRITE) {
        memset(buffer, 'A' + rand() % 25, PAGE_SIZE);
    }
    unsigned pio_cnt = 1;
    struct pio* head = create_pio(FILE_NAME, 0, page_index, operation, buffer, pio_cnt);
    submit_pio(head);
    free_pio(head);
}

int main(int argc, char* argv[]) {
    // 開啟 udm-cache
    force_exit_udm_cache();
    printf("%d init rc = %d\n", getpid(), init_udm_cache());
    buffer = alloc_dma_buffer(PAGE_SIZE);
    for (int i = 0; i < ROUND; i++)
        send_pio();
    free_dma_buffer(buffer);
    // 關閉 udm-cache
    info_udm_cache();
    printf("%d exit rc = %d\n", getpid(), exit_udm_cache());
    return 0;
}