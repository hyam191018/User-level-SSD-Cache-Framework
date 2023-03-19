#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#include "work_queue.h"

#define NUM_PRODUCERS 10
#define NUM_CONSUMERS 10
#define MAX_INDEX 1000

#define test_time 100000

static char *generate_random_string(void) {
    char *full_path_name = malloc(11 * sizeof(char)); // 需要存儲的空間至少為 11 字節（10 個字元 + 1 個空字符）

    const char *charset = "abcdefghijklmnopqrstuvwxyz"; // 可以選擇的字符集
    int len = rand() % 6 + 5; // 隨機生成的長度為 5 到 10
    for (int i = 0; i < len; i++) {
        int index = rand() % 26; // 從字符集中隨機選擇一個字符
        full_path_name[i] = charset[index];
    }
    full_path_name[len] = '\0'; // 添加一個空字符，標誌字符串的結束

    return full_path_name;
}

static void *producer_thread(void *arg) {
    work_queue *wq = (work_queue *)arg;
    char *full_path_name;
	unsigned len;
    unsigned cache_page_index;

    for(int i=0;i<test_time;i++){
        full_path_name = generate_random_string();
		len = strlen(full_path_name);
		cache_page_index = rand()%MAX_INDEX;
        if (push_work(wq, full_path_name, len, &cache_page_index)) {
            //printf("Producer: %s, %u, size is %u\n", full_path_name, cache_page_index, len);
        }
		//free(full_path_name);
    }
    return NULL;
}

static void *consumer_thread(void *arg) {
    work_queue *wq = (work_queue *)arg;
    char full_path_name[MAX_PATH_SIZE];
    unsigned cache_page_index;

    for(int i=0;i<test_time;i++){
        if (pop_work(wq, full_path_name, &cache_page_index)) {
            //printf("Consumer: %s, %u\n", full_path_name, cache_page_index);
        }
    }
    return NULL;
}

int main(int argc, char* argv[]) {
	srand(time(NULL));
	printf("MAX_QUEUE_SIZE=%d\n", MAX_WORKQUEUE_SIZE);
    pthread_t producers[NUM_PRODUCERS];
    pthread_t consumers[NUM_CONSUMERS];
    work_queue wq;
    init_work_queue(&wq);

    // create producer threads
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pthread_create(&producers[i], NULL, producer_thread, &wq);
    }

    // create consumer threads
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        pthread_create(&consumers[i], NULL, consumer_thread, &wq);
    }


    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pthread_join(producers[i], NULL);
    }

    for (int i = 0; i < NUM_CONSUMERS; i++) {
        pthread_join(consumers[i], NULL);
    }

    return 0;
}