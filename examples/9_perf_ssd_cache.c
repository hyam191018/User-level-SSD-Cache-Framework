#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "cache_api.h"
#include "spdk.h"

/**
 * 使用gettimeofday分析檔案在SPDK hit時所花費的時間
 */

// 開始測量1萬次read 4KB

#define FILE_NAME "/mnt/hdd/testfile"
#define FILE_SIZE 32 * 1024 * 1024
const unsigned long long MAX_PAGE_INDEX = 8192;

static void create_file(void) {
    int fd;
    void *buffer;
    ssize_t bytes_written;
    off_t offset;

    fd = open(FILE_NAME, O_WRONLY | O_CREAT | O_DIRECT, 0644);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    if (posix_memalign(&buffer, PAGE_SIZE, PAGE_SIZE) != 0) {
        perror("posix_memalign");
        exit(EXIT_FAILURE);
    }

    for (offset = 0; offset < FILE_SIZE; offset += PAGE_SIZE) {
        bytes_written = pwrite(fd, buffer, PAGE_SIZE, offset);
        if (bytes_written != PAGE_SIZE) {
            perror("pwrite");
            exit(EXIT_FAILURE);
        }
    }

    close(fd);
    free(buffer);
}

// 4K
static void send_pio(void *buffer) {
    unsigned page_index = rand() % MAX_PAGE_INDEX;
    struct pio *head = create_pio(FILE_NAME, 0, page_index, WRITE, buffer, 1);
    submit_pio(head);
    free_pio(head);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    // 開啟 ssd-cache
    force_exit_ssd_cache();
    init_ssd_cache();

    void *buffer = alloc_dma_buffer(PAGE_SIZE);
    // 執行1000k次運算
    for (int i = 0; i < 1000000; i++) {
        send_pio(buffer);
    }

    // 關閉 ssd-cache
    free_dma_buffer(buffer);
    info_ssd_cache();
    exit_ssd_cache();
    return 0;
}