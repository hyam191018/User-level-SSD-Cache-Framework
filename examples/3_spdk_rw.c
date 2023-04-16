#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#include "spdk.h"
#include "stdinc.h"

/**
 *  @author Hyam
 *  @date 2023/04/02
 *  @brief 寫入資料、讀出資料，開多個qpair做存取
 */

#define MAX_LBA 1024
#define ROUND 1000

static void* io_func(void* arg) {
    void *buf, id;
    unsigned lba;
    for (int i = 0; i < ROUND; i++) {
        buf = alloc_dma_buffer(PAGE_SIZE);
        lba = rand() % MAX_LBA;
        sprintf(buf, "%ld", pthread_self());
        sprintf(id, "%ld", pthread_self());
        write_spdk(buf, lba, 8, IO_QUEUE);
        memset(buf, 0, PAGE_SIZE);
        read_spdk(buf, lba, 8, IO_QUEUE);
        if (strcmp(buf, id)) {
            printf("===================\n");
            printf("| TEST SUCCESS %d |", i);
            printf("===================\n");
        } else {
            printf("===================\n");
            printf("|    TEST FAIL    |\n");
            printf("===================\n");
            exit(-1);
        }
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    init_spdk();
    srand(time(NULL));
    pthread_t admin;
    pthread_create(&admin, NULL, &io_func, NULL);
    pthread_join(admin, NULL);
    exit_spdk();
}
