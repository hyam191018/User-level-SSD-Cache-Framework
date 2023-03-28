#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "cache_api.h"
#include "stdinc.h"

#define ROUND 100000  // submit次數
#define EXCEPT 70     // 期望的 hit ratio
const int MAX_PAGE_INDEX = CACHE_BLOCK_NUMBER * CACHE_BLOCK_SIZE / 1024 * 100 / EXCEPT / 4;

static void send_pio(void) {
    char* name = "test";
    unsigned page_index = rand() % MAX_PAGE_INDEX;
    operate operation = rand() % 2 ? READ : WRITE;
    char* buffer = malloc(PAGE_SIZE);
    unsigned pio_cnt = 8 - page_index % 8;
    struct pio* head = create_pio(name, page_index, operation, buffer, pio_cnt);
    submit_pio(head);
    free_pio(head);
    free(buffer);
}

int main(int argc, char* argv[]) {
    // 開啟
    printf("%d init rc = %d\n", getpid(), init_udm_cache());

    for (int i = 0; i < ROUND; i++)
        send_pio();

    // 關閉
    info_udm_cache();
    printf("%d exit rc = %d\n", getpid(), exit_udm_cache());
    return 0;
}