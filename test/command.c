#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cache_api.h"

/*
 * Author: Hyam
 * Date: 2023/03/19
 * Description: 控制程式，可分成admin(管理者)與user(使用者)
 */

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

static void sigint_handler_for_admin(int sig_num) {
    force_exit_udm_cache();
    exit(0);
}

static void sigint_handler_for_user(int sig_num) {
    free_udm_cache();
    exit(0);
}

static void admin(void) {
    force_exit_udm_cache();
    if (init_udm_cache()) {
        return;
    }
    info_udm_cache();
    printf(">> admin is running <<\n");

    char command[100];
    while (1) {
        printf("udm-cache admin > ");
        if (!fgets(command, sizeof(command), stdin)) {
            break;
        }
        // 這裡可以對使用者輸入的指令進行處理或執行
        command[strcspn(command, "\n")] = 0;  // 刪除字串中的換行符號
        if (strcmp(command, "q") == 0) {
            break;
        } else if (strcmp(command, "s") == 0) {
            send_pio();
        } else if (strcmp(command, "i") == 0) {
            info_udm_cache();
        } else {
            printf("Not a command\n");
        }
    }
    exit_udm_cache();
}

static void user(void) {
    if (link_udm_cache()) {
        free_udm_cache();
        return;
    }
    info_udm_cache();

    char command[100];
    while (1) {
        printf("udm-cache user > ");
        if (!fgets(command, sizeof(command), stdin)) {
            break;
        }
        // 這裡可以對使用者輸入的指令進行處理或執行
        command[strcspn(command, "\n")] = 0;  // 刪除字串中的換行符號
        if (strcmp(command, "q") == 0) {
            break;
        } else if (strcmp(command, "s") == 0) {
            send_pio();
        } else if (strcmp(command, "i") == 0) {
            info_udm_cache();
        } else {
            printf("Not a command\n");
        }
    }
    free_udm_cache();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s [admin/user]\n", argv[0]);
        return 1;
    }
    // 設置SIGINT信號的處理程序
    struct sigaction sig_act;

    if (strcmp(argv[1], "admin") == 0) {
        sig_act.sa_handler = sigint_handler_for_admin;
        sigemptyset(&sig_act.sa_mask);
        sig_act.sa_flags = 0;
        sigaction(SIGINT, &sig_act, NULL);
        admin();
    } else if (strcmp(argv[1], "user") == 0) {
        sig_act.sa_handler = sigint_handler_for_user;
        sigemptyset(&sig_act.sa_mask);
        sig_act.sa_flags = 0;
        sigaction(SIGINT, &sig_act, NULL);
        user();
    } else {
        printf("Invalid argument: %s\n", argv[1]);
        return 1;
    }
    return 0;
}