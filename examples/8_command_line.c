#include "cache_api.h"
#include "spdk.h"

#define FILE_NAME "testfile"

/**
 * @author Hyam
 * @date 2023/06/27
 * @brief Server: init -> waiting -> exit
 *        Client: link -> submit  -> unlink
 *        執行使請帶入參數 "server" 或是 "client"
 */

// 隨機4K讀寫
static void send_pio(void* buffer) {
    unsigned page_index = rand() % 24;  // 三個cache block
    operate operation = rand() % 2 ? READ : WRITE;
    printf("%sing cache page %d\n", operation ? "Writ" : "Read", page_index >> 3);
    if (operation == WRITE) {
        memset(buffer, 'A' + rand() % 25, PAGE_SIZE);
    }
    unsigned pio_cnt = 1;
    struct pio* head = create_pio(FILE_NAME, 0, page_index, operation, buffer, pio_cnt);
    submit_pio(head);
    free_pio(head);
}

static void client(void) {
    if (link_ssd_cache()) {
        printf("No server is running\n");
        return;
    }
    void* buf = alloc_dma_buffer(PAGE_SIZE);
    char input[100];

    while (1) {
        printf("請輸入要傳送的訊息（輸入 \"exit\" 離開 \"send\" 請求 \"info\" 狀態）：");
        if (!fgets(input, sizeof(input), stdin)) {
            printf("錯誤:fgets\n");
        }
        input[strcspn(input, "\n")] = '\0';  // 移除換行符號

        if (strcmp(input, "exit") == 0) {
            printf("收到 exit 指令，結束 Client\n");
            break;
        } else if (strcmp(input, "send") == 0) {
            send_pio(buf);
        } else if (strcmp(input, "info") == 0) {
            info_ssd_cache();
        }
    }
    free_dma_buffer(buf);
    unlink_ssd_cache();
}

static void server(void) {
    // 強制終止先前執行的程式
    force_exit_ssd_cache();
    if (init_ssd_cache()) {
        printf("Server is running\n");
        return;
    }
    void* buf = alloc_dma_buffer(PAGE_SIZE);
    char input[100];

    while (1) {
        printf("請輸入要傳送的訊息（輸入 \"exit\" 離開 \"send\" 請求 \"info\" 狀態）：");
        if (!fgets(input, sizeof(input), stdin)) {
            printf("錯誤:fgets\n");
        }
        input[strcspn(input, "\n")] = '\0';  // 移除換行符號

        if (strcmp(input, "exit") == 0) {
            printf("收到 exit 指令，結束 Server\n");
            break;
        } else if (strcmp(input, "send") == 0) {
            send_pio(buf);
        } else if (strcmp(input, "info") == 0) {
            info_ssd_cache();
        }
    }
    free_dma_buffer(buf);
    exit_ssd_cache();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("錯誤：未提供選項！程式終止。\n");
        return 1;
    }

    char* option = argv[1];

    if (strcmp(option, "client") == 0) {
        client();
    } else if (strcmp(option, "server") == 0) {
        server();
    } else {
        printf("錯誤：無效的選項！程式終止。\n");
        return 1;
    }

    return 0;
}