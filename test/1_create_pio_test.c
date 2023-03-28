#include <stdio.h>
#include <stdlib.h>

#include "pio.h"

/*
 *  Author: Hyam
 *  Date: 2023/03/28
 *  Description: 測試 pio
 */

int main(void) {
    void *buffer = malloc(4096);
    void *buffer2 = malloc(4096);
    void *buffer3 = malloc(4096);

    // 創建PIO
    struct pio *pio = create_pio("test", 0, 0, READ, buffer, 3);

    append_pio(pio, buffer2);
    append_pio(pio, buffer3);

    // 輸出PIO的內容
    print_pio(pio);

    // 釋放PIO對象及內存空間
    free_pio(pio);
    free(buffer);
    free(buffer2);
    free(buffer3);

    return 0;
}