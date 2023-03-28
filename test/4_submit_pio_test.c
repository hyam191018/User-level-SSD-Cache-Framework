#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "cache_api.h"
#include "stdinc.h"

#define ROUND 1000000  // submit次數
#define EXCEPT 50      // 期望的 hit ratio
const unsigned long long MAX_PAGE_INDEX =
    (CACHE_BLOCK_NUMBER * CACHE_BLOCK_SIZE * 100ull) / (1024 * EXCEPT * 4ull);

// 隨機4K讀寫
static void send_pio(void) {
    // 請確保檔案存在
    char* name = "testfile";
    unsigned page_index = rand() % MAX_PAGE_INDEX;
    operate operation = rand() % 2 ? READ : WRITE;
    char* buffer = malloc(PAGE_SIZE);
    unsigned pio_cnt = 1;
    struct pio* head = create_pio(name, 0, page_index, operation, buffer, pio_cnt);
    submit_pio(head);
    free_pio(head);
    free(buffer);
}

int main(int argc, char* argv[]) {
    // 開啟 udm-cache
    force_exit_udm_cache();
    printf("%d init rc = %d\n", getpid(), init_udm_cache());

    for (int i = 0; i < ROUND; i++)
        send_pio();

    // 關閉 udm-cache
    info_udm_cache();
    printf("%d exit rc = %d\n", getpid(), exit_udm_cache());
    return 0;
}