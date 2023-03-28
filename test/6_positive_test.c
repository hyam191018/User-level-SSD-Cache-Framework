#include <sys/sem.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "cache_api.h"
#include "stdinc.h"

#define NUM_PAGE 24

// 建立一個從A開始，每個字母4KB的檔案，大小為24*4KB
static void build(void) {
    remove("ptestfile");

    int fd;
    void* buffer;
    ssize_t bytes_written;
    off_t offset = 0;

    fd = open("ptestfile", O_WRONLY | O_CREAT | O_DIRECT, 0644);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    if (posix_memalign(&buffer, PAGE_SIZE, PAGE_SIZE) != 0) {
        perror("posix_memalign");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NUM_PAGE; i++) {
        memset(buffer, i + 65, PAGE_SIZE);
        bytes_written = pwrite(fd, buffer, PAGE_SIZE, offset);
        offset += PAGE_SIZE;
        if (bytes_written != PAGE_SIZE) {
            perror("pwrite");
            exit(EXIT_FAILURE);
        }
    }

    close(fd);
    free(buffer);
}

static void test(void) {
    // 讀24 * 4KB
    char* name = "ptestfile";
    void* buffer = NULL;
    if (posix_memalign(&buffer, PAGE_SIZE, PAGE_SIZE) != 0) {
        perror("posix_memalign");
        exit(EXIT_FAILURE);
    }
    unsigned pio_cnt = 1;
    struct pio* head = NULL;
    for (int i = 0; i < 24; i++) {
        head = create_pio(name, 0, i, READ, buffer, pio_cnt);
        submit_pio(head);
        if (((char*)buffer)[0] != i + 'A') {
            printf("test fail\n");
            return;
        }
        free_pio(head);
    }
    // 寫24 * 4KB
    for (int i = 0; i < 24; i++) {
        memset(buffer, (i + 'a'), PAGE_SIZE);
        head = create_pio(name, 0, i, WRITE, buffer, pio_cnt);
        submit_pio(head);
        free_pio(head);
    }
    free(buffer);
    // 讀24 * 4KB
    for (int i = 0; i < 24; i++) {
        head = create_pio(name, 0, i, READ, buffer, pio_cnt);
        submit_pio(head);
        if (((char*)buffer)[0] != i + 'a') {
            printf("test fail\n");
            return;
        }
        free_pio(head);
    }
    printf("Test success\n");
}

int main(int argc, char* argv[]) {
    // 開啟udm-cache
    force_exit_udm_cache();
    printf("%d init rc = %d\n", getpid(), init_udm_cache());

    build();
    test();

    // 關閉udm-cache
    printf("%d exit rc = %d\n", getpid(), exit_udm_cache());
    return 0;
}