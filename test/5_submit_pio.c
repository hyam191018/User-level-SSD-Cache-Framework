#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cache_api.h"

#define USER_NUMBER 1

static void send_pio(void) {
    char* name = "test";
    unsigned page_index = 0;
    operate operation = WRITE;
    char* buffer = malloc(PAGE_SIZE);
    unsigned pio_cnt = 0;
    struct pio* head = create_pio(name, page_index, operation, buffer, pio_cnt);
    submit_pio(head);
    free_pio(head);
    free(buffer);
}

static void function(void) {
    printf("%d link rc %d\n", getpid(), link_udm_cache());
    printf("%d free rc %d\n", getpid(), free_udm_cache());
}

int main(int argc, char* argv[]) {
    pid_t pid[USER_NUMBER];  // 宣告子進程pid的陣列

    force_exit_udm_cache();
    printf("%d init rc %d\n", getpid(), init_udm_cache());

    // 使用迴圈建立多個子進程
    for (int i = 0; i < USER_NUMBER; i++) {
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
    for (int i = 0; i < USER_NUMBER; i++) {
        waitpid(pid[i], NULL, 0);
    }
    info_udm_cache();
    printf("%d exit rc %d\n", getpid(), exit_udm_cache());
    return 0;
}