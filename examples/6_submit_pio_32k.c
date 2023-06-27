#include "cache_api.h"
#include "spdk.h"

/**
 *  @author Hyam
 *  @date 2023/04/13
 *  @brief 壓力測試 submit pio 32KB
 */

// 100w  50% 11.5min
// user vector -> 100w 50% 5.5min
#define TEST_ROUND 200000
#define EXCEPT_HIT_RATIO 200
#define FILE_NAME "testfile"
const unsigned long long MAX_PAGE_INDEX =
    (CACHE_BLOCK_NUMBER * CACHE_BLOCK_SIZE * 100ull) / (1024 * EXCEPT_HIT_RATIO * 4ull);

// 隨機32K讀寫
static void send_pio(void** buffer) {
    unsigned page_index = rand() % MAX_PAGE_INDEX;
    unsigned remainder = page_index % 8;
    unsigned start_page = remainder <= 4 ? page_index - remainder : page_index + (8 - remainder);
    operate operation = rand() % 2 ? WRITE : READ;
    if (operation == WRITE) {
        for (int i = 0; i < 8; i++) {
            memset(buffer[i], 'A' + rand() % 25, PAGE_SIZE);
        }
    }
    unsigned pio_cnt = 8;
    struct pio* head = create_pio(FILE_NAME, 0, start_page, operation, buffer[0], pio_cnt);
    for (int i = 1; i < 8; i++) {
        append_pio(head, buffer[i]);
    }
    submit_pio(head);
    free_pio(head);
}

int main(int argc, char* argv[]) {
    // 開啟 ssd-cache
    force_exit_ssd_cache();
    printf("%d init rc = %d\n", getpid(), init_ssd_cache());

    void* buffer[8];
    for (int i = 0; i < 8; i++) {
        buffer[i] = alloc_dma_buffer(PAGE_SIZE);
    }
    for (int i = 0; i < TEST_ROUND; i++) {
        send_pio(buffer);
    }
    for (int i = 0; i < 8; i++) {
        free_dma_buffer(buffer[i]);
    }

    // 關閉 ssd-cache
    info_ssd_cache();
    printf("%d exit rc = %d\n", getpid(), exit_ssd_cache());
    return 0;
}