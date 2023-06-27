#include "cache_api.h"
#include "spdk.h"

/**
 *  @author Hyam
 *  @date 2023/04/13
 *  @date 2023/05/29 (modify)
 *  @brief 壓力測試 submit pio 4KB
 */

// 100w  50% 301s
// 100w 100% 65s
// 100w 200% 55s
// after iovector
// 100w  50% 219s (應該要更慢才對吧? 測量日期問題?)
// 100w 100% 64s
// 100w 200% 54s
#define TEST_ROUND 200000
#define EXCEPT_HIT_RATIO 200
#define FILE_NAME "testfile"
const unsigned long long MAX_PAGE_INDEX =
    (CACHE_BLOCK_NUMBER * CACHE_BLOCK_SIZE * 100ull) / (1024 * EXCEPT_HIT_RATIO * 4ull);

// 隨機4K讀寫
static void send_pio(void* buffer) {
    unsigned page_index = rand() % MAX_PAGE_INDEX;
    operate operation = rand() % 2 ? READ : WRITE;
    if (operation == WRITE) {
        memset(buffer, 'A' + rand() % 25, PAGE_SIZE);
    }
    unsigned pio_cnt = 1;
    struct pio* head = create_pio(FILE_NAME, 0, page_index, operation, buffer, pio_cnt);
    submit_pio(head);
    free_pio(head);
}

int main(int argc, char* argv[]) {
    // 開啟 ssd-cache
    force_exit_ssd_cache();
    printf("%d init rc = %d\n", getpid(), init_ssd_cache());

    void* buffer = alloc_dma_buffer(PAGE_SIZE);
    for (int i = 0; i < TEST_ROUND; i++) {
        send_pio(buffer);
    }
    free_dma_buffer(buffer);

    // 關閉 ssd-cache
    info_ssd_cache();
    printf("%d exit rc = %d\n", getpid(), exit_ssd_cache());
    return 0;
}