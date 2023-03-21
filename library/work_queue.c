#include "work_queue.h"

void init_work_queue(work_queue* wq) {
    wq->front = 0;
    wq->rear = 0;
    wq->size = 0;
    spinlock_init(&wq->lock);
}

static bool is_empty(work_queue* wq) {
    return (wq->front == wq->rear);
}

static bool is_full(work_queue* wq) {
    return (wq->front == (wq->rear + 1) % MAX_WORKQUEUE_SIZE);
}

static bool is_contain(work_queue* wq, char* full_path_name, unsigned cache_page_index) {
    if (is_empty(wq)) {
        return false;
    }

    for (int i = 0; i < MAX_WORKQUEUE_SIZE; i++) {
        if (wq->work_queue[i].cache_page_index == cache_page_index
            && strcmp(wq->work_queue[i].full_path_name, full_path_name) == 0) {
            return true;
        }
    }

    return false;
}

bool insert_work(work_queue* wq, char* full_path_name, unsigned path_size, unsigned cache_page_index) {
    spinlock_lock(&wq->lock);
    if (is_full(wq) || is_contain(wq, full_path_name, cache_page_index)) {
        spinlock_unlock(&wq->lock);
        return false;
    }
    
    wq->rear = (wq->rear + 1) % MAX_WORKQUEUE_SIZE;
    wq->size++;
    
    strncpy(wq->work_queue[wq->rear].full_path_name, full_path_name, path_size);
    wq->work_queue[wq->rear].full_path_name[path_size] = '\0';
    wq->work_queue[wq->rear].path_size = path_size + 1;
    wq->work_queue[wq->rear].cache_page_index = cache_page_index;
    
    spinlock_unlock(&wq->lock);
    return true;
}

bool peak_work(work_queue* wq, char* full_path_name, unsigned* cache_page_index) {
    spinlock_lock(&wq->lock);
    if (is_empty(wq)) {
        spinlock_unlock(&wq->lock);
        return false;
    }

    wq->front = (wq->front + 1) % MAX_WORKQUEUE_SIZE;
    strncpy(full_path_name, wq->work_queue[wq->front].full_path_name, wq->work_queue[wq->front].path_size);
    *cache_page_index = wq->work_queue[wq->front].cache_page_index;
    wq->front = (wq->front - 1) % MAX_WORKQUEUE_SIZE;
    
    spinlock_unlock(&wq->lock);
    return true;
}

bool remove_work(work_queue* wq) {
    spinlock_lock(&wq->lock);
    if (is_empty(wq)) {
        spinlock_unlock(&wq->lock);
        return false;
    }

    wq->front = (wq->front + 1) % MAX_WORKQUEUE_SIZE;
    
    spinlock_unlock(&wq->lock);
    return true;
}