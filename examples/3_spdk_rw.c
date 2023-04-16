#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#include "spdk.h"
#include "stdinc.h"

/**
 *  @author Hyam
 *  @date 2023/04/02
 *  @brief 寫入資料、讀出資料
 */

#define ROUND 1000
#define MAX_LBA 100

static void test_func(queue_type type) {
    void *buf, *tid;
    unsigned lba;
    buf = alloc_dma_buffer(PAGE_SIZE);
    tid = malloc(PAGE_SIZE);
    for (int i = 0; i < ROUND; i++) {
        lba = rand() % MAX_LBA;
        sprintf(buf, "%ld", rand() % 1000000);
        sprintf(tid, "%ld", rand() % 1000000);
        write_spdk(buf, lba, 8, type);
        memset(buf, 0, PAGE_SIZE);
        read_spdk(buf, lba, 8, type);
        trim_spdk(lba, 8, type);
        if (strcmp(buf, tid)) {
            printf("===================\n");
            printf("|    TEST FAIL    |\n");
            printf("===================\n");
            exit(-1);
        }
    }
    free_dma_buffer(buf);
    free(tid);
}

int main(int argc, char* argv[]) {
    init_spdk();
    srand(time(NULL));
    test_func(IO_QUEUE);
    printf("===================\n");
    printf("|  TEST SUCCESS   |\n");
    printf("===================\n");
    exit_spdk();
}
