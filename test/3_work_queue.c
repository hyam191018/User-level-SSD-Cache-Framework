#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include "work_queue.h"

#define MAX_WORKS 100000
#define NUM_PRODUCERS 10

pthread_mutex_t isdone_mutex = PTHREAD_MUTEX_INITIALIZER;
int producer_done_count = 0;
bool isdone = false;

static void *producer(void *arg)
{
    work_queue *wq = (work_queue *)arg;
    int time = 0;
    while (true)
    {
        char full_path_name[MAX_PATH_SIZE + 1];
        unsigned cp = rand() % 10;
        sprintf(full_path_name, "/path/to/work/%d", cp);
        if (insert_work(wq, full_path_name, strlen(full_path_name), cp))
        {
            printf("Insert a work\n");
            time++;
            if (time == MAX_WORKS)
            {
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

static void *consumer(void *arg)
{
    work_queue *wq = (work_queue *)arg;
    while (!isdone)
    {
        char full_path_name[MAX_PATH_SIZE + 1];
        unsigned cache_page_index;
        if (peak_work(wq, full_path_name, &cache_page_index))
        {
            printf("Remove a work\n");
            remove_work(wq);
        }
    }
    return NULL;
}

static void test_work_queue(void)
{
    work_queue wq;
    init_work_queue(&wq);

    pthread_t producers[NUM_PRODUCERS];
    for (int i = 0; i < NUM_PRODUCERS; i++)
    {
        pthread_create(&producers[i], NULL, producer, &wq);
    }

    pthread_t consumer_thread;
    pthread_create(&consumer_thread, NULL, consumer, &wq);

    for (int i = 0; i < NUM_PRODUCERS; i++)
    {
        pthread_join(producers[i], NULL);
    }

    pthread_join(consumer_thread, NULL);
}

int main(int argc, char *argv[])
{
    test_work_queue();
    return 0;
}