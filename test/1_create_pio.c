#include "pio.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    // 分別申請三塊4096字節的內存空間
    void* buffer = malloc(4096);
    void* buffer2 = malloc(4096);
    void* buffer3 = malloc(4096);

    // 創建PIO對象
    struct pio* pio = create_pio("test", 0, READ, buffer, 3);

    // 追加兩塊內存空間到PIO中
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