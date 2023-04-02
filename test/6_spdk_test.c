#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "spdk.h"
#include "stdinc.h"

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
    exit_spdk();
}
