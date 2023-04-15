#include <stdio.h>
#include <stdlib.h>

#include "pio.h"

/**
 *  @author Hyam
 *  @date 2023/03/28
 *  @brief 測試 pio
 */

int main(void) {
    void *buffer1 = malloc(4096);
    void *buffer2 = malloc(4096);
    void *buffer3 = malloc(4096);

    // 建立PIO
    struct pio *pio = create_pio("test", 0, 0, READ, buffer1, 3);
    append_pio(pio, buffer2);
    append_pio(pio, buffer3);

    // 輸出PIO的內容
    print_pio(pio);

    // 釋放PIO
    free_pio(pio);
    free(buffer1);
    free(buffer2);
    free(buffer3);

    return 0;
}