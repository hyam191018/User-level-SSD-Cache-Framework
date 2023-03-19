#include "work_queue.h"

void init_work_queue(work_queue* wq) {
    wq->front = 0;
    wq->rear = -1;
    wq->size = 0;
    spinlock_init(&wq->lock);
}

bool is_empty_work_queue(work_queue* wq) {
    bool empty;
    spinlock_lock(&wq->lock);
    empty = (wq->size == 0);
    spinlock_unlock(&wq->lock);
    return empty;
}

bool is_full_work_queue(work_queue* wq) {
    bool full;
    spinlock_lock(&wq->lock);
    full = (wq->size == MAX_WORKQUEUE_SIZE);
    spinlock_unlock(&wq->lock);
    return full;
}

bool push_work(work_queue* wq, char* full_path_name, unsigned *cache_page_index) {
    if (is_full_work_queue(wq)) {
        return false;
    }
    spinlock_lock(&wq->lock);
    wq->rear = (wq->rear + 1) % MAX_WORKQUEUE_SIZE;
    strncpy(wq->work_queue[wq->rear].full_path_name, full_path_name, MAX_PATH_SIZE);
    wq->work_queue[wq->rear].cache_page_index = *cache_page_index;
    wq->size++;
    spinlock_unlock(&wq->lock);
    return true;
}

bool pop_work(work_queue* wq, char* full_path_name, unsigned *cache_page_index) {
    if (is_empty_work_queue(wq)) {
        return false;
    }
    spinlock_lock(&wq->lock);
    strncpy(full_path_name, wq->work_queue[wq->front].full_path_name, MAX_PATH_SIZE);
    *cache_page_index = wq->work_queue[wq->front].cache_page_index;
    wq->front = (wq->front + 1) % MAX_WORKQUEUE_SIZE;
    wq->size--;
    spinlock_unlock(&wq->lock);
    return true;
}