#include <malloc.h>

#include "cache_api.h"
#include "spdk.h"

#define FILE_SIZE (1 << 18)  // PAGES

static void ssd_cache(void) {
    if (link_ssd_cache()) {
        printf("No server is running\n");
        return;
    }

    void* buf = alloc_dma_buffer(PAGE_SIZE);
    memset(buf, 'A', PAGE_SIZE);
    struct pio* head = NULL;
    for (int i = 0; i < 1000000; i++) {
        head = create_pio("testfile", 0, rand() % FILE_SIZE, WRITE, buf, 1);
        submit_pio(head);
    }

    free_dma_buffer(buf);
    unlink_ssd_cache();
}

static void dm_cache(void) {
    char* buf = memalign(PAGE_SIZE * 2, PAGE_SIZE * 2);
    memset(buf, 'A', PAGE_SIZE);
    buf += PAGE_SIZE;
    int offset = 0;
    int bytes = 0;
    int fd = open("/mnt/cache/testfile", O_RDWR | O_DIRECT, 0644);
    for (int i = 0; i < 1000000; i++) {
        offset = (rand() % FILE_SIZE) << 12;
        bytes = pwrite(fd, buf, PAGE_SIZE, offset);
        if (bytes != PAGE_SIZE) {
            perror("IO fail");
            break;
        }
    }
    close(fd);
}

int main(int argc, char* argv[]) {
    srand(time(NULL));
    // ssd_cache();
    dm_cache();
    return 0;
}