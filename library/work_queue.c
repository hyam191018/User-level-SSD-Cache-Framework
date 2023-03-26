#include "work_queue.h"

void init_work_queue(work_queue *wq) {
    wq->front = 0;
    wq->rear = 0;
    wq->size = 0;
    spinlock_init(&wq->lock);
}

bool is_full(work_queue *wq) {
    spinlock_lock(&wq->lock);
    bool res = (wq->size == MAX_WORKQUEUE_SIZE);
    spinlock_unlock(&wq->lock);
    return res;
}

bool is_empty(work_queue *wq) {
    spinlock_lock(&wq->lock);
    bool res = (wq->size == 0);
    spinlock_unlock(&wq->lock);
    return res;
}

bool contains_work(work_queue *wq, char *full_path_name, unsigned cache_page_index) {
    spinlock_lock(&wq->lock);
    for (int i = wq->front; i != wq->rear; i = (i + 1) % MAX_WORKQUEUE_SIZE) {
        if (wq->works[i].cache_page_index == cache_page_index &&
            strncmp(wq->works[i].full_path_name, full_path_name,
                    strlen(wq->works[i].full_path_name)) == 0) {
            spinlock_unlock(&wq->lock);
            return true;
        }
    }
    spinlock_unlock(&wq->lock);
    return false;
}

bool insert_work(work_queue *wq, char *full_path_name, unsigned cache_page_index) {
    spinlock_lock(&wq->lock);
    /* queue is full */
    if (wq->size == MAX_WORKQUEUE_SIZE - 1) {
        spinlock_unlock(&wq->lock);
        return false;
    }
    /* check whether it is queued */
    for (int i = wq->front; i != wq->rear; i = (i + 1) % MAX_WORKQUEUE_SIZE) {
        if (wq->works[i].cache_page_index == cache_page_index &&
            strcmp(wq->works[i].full_path_name, full_path_name) == 0) {
            spinlock_unlock(&wq->lock);
            return false;
        }
    }
    // printf("Insert work:%s %u\n", full_path_name, cache_page_index);
    strcpy(wq->works[wq->rear].full_path_name, full_path_name);
    wq->works[wq->rear].cache_page_index = cache_page_index;
    wq->rear = (wq->rear + 1) % MAX_WORKQUEUE_SIZE;
    wq->size++;
    spinlock_unlock(&wq->lock);
    return true;
}

bool remove_work(work_queue *wq) {
    spinlock_lock(&wq->lock);
    /* queue is empty */
    if (wq->size == 0) {
        spinlock_unlock(&wq->lock);
        return false;
    }
    wq->front = (wq->front + 1) % MAX_WORKQUEUE_SIZE;
    wq->size--;
    spinlock_unlock(&wq->lock);
    return true;
}

bool peak_work(work_queue *wq, char *full_path_name, unsigned *cache_page_index) {
    spinlock_lock(&wq->lock);
    /* queue is empty */
    if (wq->size == 0) {
        spinlock_unlock(&wq->lock);
        return false;
    }
    strcpy(full_path_name, wq->works[wq->front].full_path_name);
    *cache_page_index = wq->works[wq->front].cache_page_index;
    // printf("get work:%s %u\n", full_path_name, *cache_page_index);
    spinlock_unlock(&wq->lock);
    return true;
}