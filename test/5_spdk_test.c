#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "spdk.h"
#include "stdinc.h"

/**
 *  Author: Hyam
 *  Date: 2023/04/02
 *  Description: SPDK 正向測試 寫入資料、讀出資料，multi-thread競爭
 */

static void* user_func(void* arg) {
    void* buf = alloc_dma_buffer(PAGE_SIZE);
    sprintf(buf, "I am pthread %ld.\n", pthread_self());
    write_spdk(buf, 0, 8);
    memset(buf, 0, PAGE_SIZE);
    read_spdk(buf, 0, 8);
    printf("%ld say: %s", pthread_self(), (char*)buf);
    return NULL;
}

int main(int argc, char* argv[]) {
    init_spdk();
    const int user_num = 100;
    pthread_t user[user_num];
    for (int i = 0; i < user_num; i++) {
        pthread_create(&user[i], NULL, &user_func, NULL);
    }
    for (int i = 0; i < user_num; i++) {
        pthread_join(user[i], NULL);
    }
    trim_spdk(0, 8);
    exit_spdk();
}
