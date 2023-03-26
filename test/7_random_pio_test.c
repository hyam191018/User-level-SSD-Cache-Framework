#include <sys/types.h>
#include <sys/wait.h>

#include "cache_api.h"
#include "stdinc.h"

// 製作pio
static void send_pio(void) {
    char* name = "test";
    unsigned page_index = 0;
    operate operation = WRITE;
    char* buffer = malloc(PAGE_SIZE);
    unsigned pio_cnt = 8;
    struct pio* head = create_pio(name, page_index, operation, buffer, pio_cnt);
    submit_pio(head);
    free_pio(head);
    free(buffer);
}

static void user(void) {
    printf("I am user %d\n", getpid());
    sleep(1);
    printf("%d link rc = %d\n", getpid(), link_udm_cache());
    send_pio();
    printf("%d free rc = %d\n", getpid(), free_udm_cache());
}

static void admin(void) {
    printf("I am admin %d\n", getpid());
    printf("%d init rc = %d\n", getpid(), init_udm_cache());
    sleep(3);
    printf("%d exit rc = %d\n", getpid(), exit_udm_cache());
}

int main(int argc, char* argv[]) {
    pid_t pid1, pid2;

    pid1 = fork();  // 建立子進程1
    if (pid1 == 0) {
        // 子進程1執行程式A
        user();
        exit(1);
    }

    pid2 = fork();  // 建立子進程2
    if (pid2 == 0) {
        // 子進程2執行程式B
        admin();
        exit(1);
    }

    // 等待兩個子進程結束
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return 0;
}