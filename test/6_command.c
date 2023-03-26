#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cache_api.h"

/*
 * Author: Hyam
 * Date: 2023/03/25
 * Description: 控制程式，可分成admin(管理者)與user(使用者)
 */

// 製作pio
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

static void admin(void) {
    printf("%d init rc = %d\n", getpid(), init_udm_cache());
    // 開始執行命令列
    char command[100];
    while (1) {
        printf("udm-cache admin > ");
        if (!fgets(command, sizeof(command), stdin)) {
            break;
        }
        // 這裡可以對使用者輸入的指令進行處理或執行
        command[strcspn(command, "\n")] = 0;  // 刪除字串中的換行符號
        if (strcmp(command, "send") == 0) {
            send_pio();
        } else if (strcmp(command, "info") == 0) {
            info_udm_cache();
        } else if (strcmp(command, "quit") == 0) {
            break;
        } else {
            printf("Not a command\n");
        }
        sleep(1);
    }
    printf("%d exit rc = %d\n", getpid(), exit_udm_cache());
}

static void user(void) {
    printf("%d link rc = %d\n", getpid(), link_udm_cache());
    // 開始執行命令列
    char command[100];
    while (1) {
        printf("udm-cache user > ");
        if (!fgets(command, sizeof(command), stdin)) {
            break;
        }
        // 這裡可以對使用者輸入的指令進行處理或執行
        command[strcspn(command, "\n")] = 0;  // 刪除字串中的換行符號
        if (strcmp(command, "send") == 0) {
            send_pio();
        } else if (strcmp(command, "info") == 0) {
            info_udm_cache();
        } else if (strcmp(command, "quit") == 0) {
            break;
        } else {
            printf("Not a command\n");
        }
        sleep(1);
    }
    printf("%d free rc = %d\n", getpid(), free_udm_cache());
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s [admin/user/clear]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "admin") == 0) {
        admin();
    } else if (strcmp(argv[1], "user") == 0) {
        user();
    } else if (strcmp(argv[1], "clear") == 0) {
        force_exit_udm_cache();
    } else {
        printf("Invalid argument: %s\n", argv[1]);
        return 1;
    }
    return 0;
}