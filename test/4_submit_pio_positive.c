#include "cache_api.h"
#include "spdk.h"
#include "stdinc.h"

/**
 *  @author Hyam
 *  @date 2023/04/13
 *  @brief 正向測試 admin submit pio
 */

#define FILE_NAME "testfile"

int main(int argc, char* argv[]) {
    // 開啟 udm-cache
    printf("%d init rc = %d\n", getpid(), init_udm_cache());

    /* how to test? */

    // 關閉 udm-cache
    info_udm_cache();
    printf("%d exit rc = %d\n", getpid(), exit_udm_cache());
    return 0;
}