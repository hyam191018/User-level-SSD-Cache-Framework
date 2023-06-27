#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <time.h>

#include "spdk.h"

/**
 *  @author Hyam
 *  @date 2023/05/29
 *  @brief IO Vector
 */

#define PAGE_SIZE 4096

int main(int argc, char* argv[]) {
    init_spdk();
    // 測試 write > read
    printf("test: write > read\n");
    void* buf = alloc_dma_buffer(PAGE_SIZE * 8);
    for (int i = 0; i < 8; i++) {
        memset(buf + i * PAGE_SIZE, 'A' + i, PAGE_SIZE);
    }
    write_spdk(buf, 0, 64, 0);
    memset(buf, 0, PAGE_SIZE * 8);
    read_spdk(buf, 0, 64, 0);
    for (int i = 0; i < 8; i++) {
        printf("%d is %c\n", i + 1, ((char*)buf)[i * PAGE_SIZE]);
    }
    // 測試 writev > read
    printf("test: writev > read\n");
    struct iovec iovs[8];
    for (int i = 0; i < 8; i++) {
        iovs[i].iov_base = alloc_dma_buffer(PAGE_SIZE);
        iovs[i].iov_len = PAGE_SIZE;
        memset(iovs[i].iov_base, 'a' + i, PAGE_SIZE);
    }
    writev_spdk(iovs, 8, 0, 64, 0);
    read_spdk(buf, 0, 64, 0);
    for (int i = 0; i < 8; i++) {
        printf("%d is %c\n", i + 1, ((char*)buf)[i * PAGE_SIZE]);
    }
    // 測試 write > readv
    printf("test: write > readv\n");
    for (int i = 0; i < 8; i++) {
        memset(buf + i * PAGE_SIZE, 'A' + i, PAGE_SIZE);
    }
    write_spdk(buf, 0, 64, 0);
    readv_spdk(iovs, 8, 0, 64, 0);
    for (int i = 0; i < 8; i++) {
        printf("%d is %c\n", i + 1, ((char*)iovs[i].iov_base)[0]);
    }
    exit_spdk();
}
