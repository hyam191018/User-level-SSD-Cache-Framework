#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "cache_api.h"
#include "spdk.h"
#include "stdinc.h"

/**
 *  Author: Hyam
 *  Date: 2023/03/28
 *  Description: 正向測試 建立檔案>讀取檔案>寫入檔案>讀取檔案
 */

#define NUM_PAGE 24

// 建立一個從A開始，每個字母4KB的檔案，大小為24*4KB
static void build(void) {
    int fd;
    void* buffer;
    ssize_t bytes_written;
    off_t offset = 0;

    fd = open("ptestfile", O_WRONLY | O_CREAT | O_DIRECT, 0644);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    if (posix_memalign(&buffer, PAGE_SIZE, PAGE_SIZE) != 0) {
        perror("posix_memalign");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NUM_PAGE; i++) {
        memset(buffer, i + 65, PAGE_SIZE);
        bytes_written = pwrite(fd, buffer, PAGE_SIZE, offset);
        offset += PAGE_SIZE;
        if (bytes_written != PAGE_SIZE) {
            perror("pwrite");
            exit(EXIT_FAILURE);
        }
    }

    close(fd);
    free(buffer);
}

static void test(void) {
    // 讀A~X
    char* name = "ptestfile";
    void* buffer = alloc_dma_buffer(PAGE_SIZE);
    unsigned pio_cnt = 1;
    struct pio* head = NULL;
    printf("Reading:");
    for (int i = 0; i < 24; i++) {
        head = create_pio(name, 0, i, READ, buffer, pio_cnt);
        submit_pio(head);
        printf("%c", ((char*)buffer)[0]);
        free_pio(head);
    }
    printf("\n");

    // 寫a~x
    printf("Writing:");
    for (int i = 0; i < 24; i++) {
        memset(buffer, (i + 'a'), PAGE_SIZE);
        printf("%c", ((char*)buffer)[0]);
        head = create_pio(name, 0, i, WRITE, buffer, pio_cnt);
        submit_pio(head);
        free_pio(head);
    }
    printf("\n");

    // 讀a~x
    memset(buffer, 0, PAGE_SIZE);
    printf("Reading:");
    for (int i = 0; i < 24; i++) {
        head = create_pio(name, 0, i, READ, buffer, pio_cnt);
        submit_pio(head);
        printf("%c", ((char*)buffer)[0]);
        free_pio(head);
    }
    printf("\n");
    sleep(1);  // 確保migration完成

    // 從SSD讀a~x
    memset(buffer, 0, PAGE_SIZE);
    printf("Reading:");
    for (int i = 0; i < 24; i++) {
        head = create_pio(name, 0, i, READ, buffer, pio_cnt);
        submit_pio(head);
        printf("%c", ((char*)buffer)[0]);
        free_pio(head);
    }
    printf("\n");

    // 寫A~X到SSD
    printf("Writing:");
    for (int i = 0; i < 24; i++) {
        memset(buffer, (i + 'A'), PAGE_SIZE);
        printf("%c", ((char*)buffer)[0]);
        head = create_pio(name, 0, i, WRITE, buffer, pio_cnt);
        submit_pio(head);
        free_pio(head);
    }
    printf("\n");

    // 讀A~X
    memset(buffer, 0, PAGE_SIZE);
    printf("Reading:");
    for (int i = 0; i < 24; i++) {
        head = create_pio(name, 0, i, READ, buffer, pio_cnt);
        submit_pio(head);
        printf("%c", ((char*)buffer)[0]);
        free_pio(head);
    }
    free_dma_buffer(buffer);
    printf("\n");
    printf("Test success\n");
}

int main(int argc, char* argv[]) {
    // 開啟udm-cache
    force_exit_udm_cache();
    printf("%d init rc = %d\n", getpid(), init_udm_cache());

    build();
    test();
    remove("ptestfile");

    // 關閉udm-cache
    printf("%d exit rc = %d\n", getpid(), exit_udm_cache());
    return 0;
}