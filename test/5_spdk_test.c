#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#include "spdk.h"
#include "stdinc.h"

/**
 *  Author: Hyam
 *  Date: 2023/04/02
 *  Description: SPDK 正向測試 寫入資料、讀出資料，multi-thread競爭
 */

#define MAX_LBA 2097152
#define ROUND 10
#define USERS 10

static void* user_func(void* arg) {
    void* buf;
    unsigned lba;
    for (int i = 0; i < ROUND; i++) {
        buf = alloc_dma_buffer(PAGE_SIZE);
        lba = rand() % MAX_LBA;
        sprintf(buf, "I am pthread %ld", pthread_self());
        write_spdk(buf, lba, 8);
        memset(buf, 0, PAGE_SIZE);
        read_spdk(buf, lba, 8);
        // printf("%s\n", (char*)buf);
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    init_spdk();
    srand(time(NULL));
    pthread_t user[USERS];
    for (int i = 0; i < USERS; i++) {
        pthread_create(&user[i], NULL, &user_func, NULL);
    }
    for (int i = 0; i < USERS; i++) {
        pthread_join(user[i], NULL);
    }
    trim_spdk(0, 8);
    exit_spdk();
}
