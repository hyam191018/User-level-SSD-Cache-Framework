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

#define FILE_NAME "/mnt/cache/testfile"
#define FILE_SIZE 32 * 1024 * 1024
const unsigned long long MAX_PAGE_INDEX = 8192;

int main(int argc, char *argv[]) {
    srand(time(NULL));
    unsigned page_index;

    int fd;
    void *buffer;
    ssize_t bytes_written;
    off_t offset;

    fd = open(FILE_NAME, O_RDWR | O_CREAT | O_DIRECT, 0644);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    if (posix_memalign(&buffer, PAGE_SIZE, PAGE_SIZE) != 0) {
        perror("posix_memalign");
        exit(EXIT_FAILURE);
    }
    memset(buffer, 'A', PAGE_SIZE);

    for (int i = 0; i < 1000000; i++) {
        page_index = rand() % MAX_PAGE_INDEX;
        bytes_written = pwrite(fd, buffer, PAGE_SIZE, page_index << 12);
        if (bytes_written != PAGE_SIZE) {
            perror("pwrite");
            exit(EXIT_FAILURE);
        }
    }

    close(fd);
    free(buffer);

    return 0;
}