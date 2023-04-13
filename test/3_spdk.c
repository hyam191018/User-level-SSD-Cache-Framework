#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#include "spdk.h"
#include "stdinc.h"

/**
 *  @author Hyam
 *  @date 2023/04/02
 *  @brief SPDK 正向測試 寫入資料、讀出資料，multi-thread競爭
 */

#define MAX_LBA 2097152
#define ROUND 1000

static void* admin_func(void* arg) {
    void* buf;
    unsigned lba;
    for (int i = 0; i < ROUND; i++) {
        buf = alloc_dma_buffer(PAGE_SIZE);
        lba = rand() % MAX_LBA;
        sprintf(buf, "I am pthread %ld", pthread_self());
        write_spdk(buf, lba, 8, IO_QUEUE);
        memset(buf, 0, PAGE_SIZE);
        read_spdk(buf, lba, 8, IO_QUEUE);
    }
    return NULL;
}

static void* mg_func(void* arg) {
    void* buf;
    unsigned lba;
    for (int i = 0; i < ROUND; i++) {
        buf = alloc_dma_buffer(PAGE_SIZE);
        lba = rand() % MAX_LBA;
        sprintf(buf, "I am pthread %ld", pthread_self());
        write_spdk(buf, lba, 8, MG_QUEUE);
        memset(buf, 0, PAGE_SIZE);
        read_spdk(buf, lba, 8, MG_QUEUE);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    init_spdk();
    srand(time(NULL));
    pthread_t admin, worker;
    pthread_create(&admin, NULL, &admin_func, NULL);
    pthread_create(&worker, NULL, &mg_func, NULL);

    pthread_join(admin, NULL);
    pthread_join(worker, NULL);

    exit_spdk();
}
