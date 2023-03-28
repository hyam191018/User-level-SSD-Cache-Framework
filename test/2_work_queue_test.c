#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "work_queue.h"

/*
 *  Author: Hyam
 *  Date: 2023/03/28
 *  Description: 壓力測試 work_queue
 */

#define MAX_WORKS 10
#define NUM_PRODUCERS 10

pthread_mutex_t isdone_mutex = PTHREAD_MUTEX_INITIALIZER;
int producer_done_count = 0;
bool isdone = false;

static char *gen(int num) {
    char *str = (char *)malloc(num + 1);  // allocate memory for the string
    srand(time(NULL));                    // seed the random number generator with the current time
    for (int i = 0; i < num; i++) {
        int rand_num = rand() % 26;  // generate a random number between 0 and 25
        str[i] = 'A' + rand_num;     // convert the random number to an uppercase letter
    }
    str[num] = '\0';  // add a null terminator to the end of the string
    return str;
}

static void *producer(void *arg) {
    work_queue *wq = (work_queue *)arg;
    int time = 0;
    while (true) {
        char *full_path_name = gen(3);
        unsigned cp = rand() % 5;
        // Insert work 的訊息可能不準確，所以是寫在work_queue.c 用lock保護
        if (insert_work(wq, full_path_name, cp)) {
            time++;
            if (time == MAX_WORKS) {
                break;
            }
        }
    }
    pthread_mutex_lock(&isdone_mutex);
    producer_done_count++;
    if (producer_done_count == NUM_PRODUCERS)
        isdone = true;
    pthread_mutex_unlock(&isdone_mutex);
    return NULL;
}

static void *consumer(void *arg) {
    work_queue *wq = (work_queue *)arg;
    while (!isdone) {
        char full_path_name[MAX_PATH_SIZE + 1];
        unsigned cache_page_index;
        if (peak_work(wq, full_path_name, &cache_page_index)) {
            printf("get work:%s %u\n", full_path_name, cache_page_index);
            remove_work(wq);
        }
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

int main(int argc, char *argv[]) {
    test_work_queue();
    return 0;
}