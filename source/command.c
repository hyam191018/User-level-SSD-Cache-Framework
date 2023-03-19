#include "cache_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/*
 * Author: Hyam
 * Date: 2023/03/19
 * Description: 控制程式，可分成admin(管理者)與user(使用者)
 */

static void sigint_handler(int sig_num) {
	shutdown_mg_worker();
	free_udm_cache();
	exit_udm_cache();
    exit(0);
}

static void admin(void) {
	exit_udm_cache();
	init_udm_cache();
	info_udm_cache();
	wakeup_mg_worker();
	wakeup_wb_worker();

	printf("( udm-cache init complete )\n");
	char input[100];
    while (1) {
        printf("udm-cache > ");
		if(fgets(input, 100, stdin) == NULL){
			printf("Error when fgets\n");
			goto end;
		}

		// 移除字串末尾的換行符號
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "stop") == 0) {
            break;
        }else{
			printf("No a command\n");
		}
    }
end:
	shutdown_wb_worker();
	shutdown_mg_worker();
	free_udm_cache();
	exit_udm_cache();
}

static void user(void) {
	link_udm_cache();
	info_udm_cache();
	free_udm_cache();
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s [admin/user]\n", argv[0]);
        return 1;
    }

	// 設置SIGINT信號的處理程序
    struct sigaction sig_act;
    sig_act.sa_handler = sigint_handler;
    sigemptyset(&sig_act.sa_mask);
    sig_act.sa_flags = 0;
    sigaction(SIGINT, &sig_act, NULL);

    if (strcmp(argv[1], "admin") == 0) {
        admin();
    } else if (strcmp(argv[1], "user") == 0) {
        user();
    } else {
        printf("Invalid argument: %s\n", argv[1]);
        return 1;
    }

    return 0;
}