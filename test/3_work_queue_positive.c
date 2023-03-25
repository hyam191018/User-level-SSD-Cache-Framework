#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "work_queue.h"

#define CACHE_PAGES 10

int main(int argc, char* argv[]) {
    work_queue wq;
    init_work_queue(&wq);

    // Insert some works
    insert_work(&wq, "/path/to/file1", 0);
    insert_work(&wq, "/path/to/file2", 1);
    insert_work(&wq, "/path/to/file3", 2);

    // Test contains_work
    if (contains_work(&wq, "/path/to/file1", 0)) {
        printf("Work 1 is in the queue.\n");
    }
    if (contains_work(&wq, "/path/to/file2", 1)) {
        printf("Work 2 is in the queue.\n");
    }
    if (contains_work(&wq, "/path/to/file3", 2)) {
        printf("Work 3 is in the queue.\n");
    }

    // Test peak_work
    char* path_name = malloc(sizeof(char) * 100);
    unsigned cache_page_index;
    if (peak_work(&wq, path_name, &cache_page_index)) {
        printf("Peaked work: %s, %u\n", path_name, cache_page_index);
    }

    // Test remove_work
    if (remove_work(&wq)) {
        printf("Removed work from queue.\n");
    }

    // Test inserting more works than queue size
    for (int i = 0; i < CACHE_PAGES * 2; i++) {
        char path_name[100];
        sprintf(path_name, "/path/to/file%d", i);
        if (!insert_work(&wq, path_name, i)) {
            printf("Work %d couldn't be inserted.\n", i);
        }
    }

    // Test inserting duplicate work
    if (!insert_work(&wq, "/path/to/file1", 0)) {
        printf("Duplicate work couldn't be inserted.\n");
    }

    return 0;
}