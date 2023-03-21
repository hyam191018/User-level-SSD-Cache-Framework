#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include "work_queue.h"

#define MAX_WORKS 10000000
#define NUM_PRODUCERS 100

static void* producer(void* arg) {
    work_queue* wq = (work_queue*)arg;
	int time = 0;
    while (true) {
        char full_path_name[MAX_PATH_SIZE + 1];
		unsigned cp = rand()%10;
        sprintf(full_path_name, "/path/to/work/%d", cp);
        if(insert_work(wq, full_path_name, strlen(full_path_name), cp)){
			time++;
			if(time == MAX_WORKS) break;
		}
    }
    return NULL;
}

static void* consumer(void* arg) {
    work_queue* wq = (work_queue*)arg;
    while (true) {
        char full_path_name[MAX_PATH_SIZE + 1];
        unsigned cache_page_index;
        if (peak_work(wq, full_path_name, &cache_page_index)) {
            remove_work(wq);
        }
	printf("FRee\n");
    }
    return NULL;
}
static void test_work_queue(void) {
    work_queue wq;
    init_work_queue(&wq);

    pthread_t producers[NUM_PRODUCERS];
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pthread_create(&producers[i], NULL, producer, &wq);
    }

    pthread_t consumer_thread;
    pthread_create(&consumer_thread, NULL, consumer, &wq);

    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pthread_join(producers[i], NULL);
    }

    pthread_join(consumer_thread, NULL);
}

int main(int argc, char* argv[]) {
    test_work_queue();
    return 0;
}