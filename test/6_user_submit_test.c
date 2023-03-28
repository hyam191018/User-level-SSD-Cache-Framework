#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cache_api.h"

#define USERS 3       // 使用者數量
#define ROUND 100000  // 使用者submit次數
#define EXCEPT 70     // 期望的 hit ratio
const int MAX_PAGE_INDEX = CACHE_BLOCK_NUMBER * CACHE_BLOCK_SIZE / 1024 * 100 / EXCEPT / 4;

static void send_pio(void) {
    char* name = "test";
    unsigned page_index = rand() % MAX_PAGE_INDEX;
    operate operation = rand() % 2 ? READ : WRITE;
    char* buffer = malloc(PAGE_SIZE);
    unsigned pio_cnt = 8 - page_index % 8;
    struct pio* head = create_pio(name, page_index, operation, buffer, pio_cnt);
    submit_pio(head);
    free_pio(head);
    free(buffer);
}

static void function(void) {}

int main(int argc, char* argv[]) {
    int num_processes = 10;    // 指定建立的子進程數量
    pid_t pid[num_processes];  // 宣告子進程pid的陣列

    force_exit_udm_cache();  // 強制清除已建立shm
    printf("%d init rc %d\n", getpid(), init_udm_cache());

    // 使用迴圈建立多個子進程
    for (int i = 0; i < num_processes; i++) {
        pid[i] = fork();
        if (pid[i] < 0) {
            fprintf(stderr, "Fork failed.\n");
            exit(1);
        } else if (pid[i] == 0) {
            function();
            exit(0);
        }
    }

    // 父進程等待所有子進程結束
    for (int i = 0; i < num_processes; i++) {
        waitpid(pid[i], NULL, 0);
    }

    info_udm_cache();
    printf("%d exit rc %d\n", getpid(), exit_udm_cache());
    return 0;
}