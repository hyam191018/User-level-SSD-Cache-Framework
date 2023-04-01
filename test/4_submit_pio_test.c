#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "cache_api.h"
#include "stdinc.h"

/**
 *  Author: Hyam
 *  Date: 2023/03/28
 *  Description: 壓力測試 admin submit pio (read only)
 */

#define ROUND 1000000  // submit次數
#define EXCEPT 100     // 期望的 hit ratio
#define FILE_NAME "testfile"
const unsigned long long MAX_PAGE_INDEX =
    (CACHE_BLOCK_NUMBER * CACHE_BLOCK_SIZE * 100ull) / (1024 * EXCEPT * 4ull);

// 建立檔案
static void build_file(void) {
    unsigned long long file_size = MAX_PAGE_INDEX * PAGE_SIZE;

    FILE* fp = fopen(FILE_NAME, "wb");
    if (fp == NULL) {
        printf("Error opening file\n");
        return;
    }

    char* buffer = (char*)malloc(PAGE_SIZE);

    for (unsigned long long i = 0; i < file_size; i += PAGE_SIZE) {
        fwrite(buffer, 1, PAGE_SIZE, fp);
    }

    fclose(fp);
    free(buffer);
}

// 隨機4K讀寫
static void send_pio(void) {
    unsigned page_index = rand() % MAX_PAGE_INDEX;
    operate operation = rand() % 2 ? READ : WRITE;
    char* buffer = malloc(PAGE_SIZE);
    unsigned pio_cnt = 1;
    struct pio* head = create_pio(FILE_NAME, 0, page_index, operation, buffer, pio_cnt);
    submit_pio(head);
    free_pio(head);
    free(buffer);
}

int main(int argc, char* argv[]) {
    // 開啟 udm-cache
    printf("%d init rc = %d\n", getpid(), init_udm_cache());
    build_file();

    for (int i = 0; i < ROUND; i++)
        send_pio();

    remove(FILE_NAME);
    // 關閉 udm-cache
    info_udm_cache();
    printf("%d exit rc = %d\n", getpid(), exit_udm_cache());
    return 0;
}