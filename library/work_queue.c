#include "work_queue.h"

void init_work_queue(work_queue* wq) {
    wq->front = 0;
    wq->rear = 0;
    wq->size = 0;
    spinlock_init(&wq->lock);
}

static bool is_full(work_queue* wq) {
    return wq->size == (MAX_WORKQUEUE_SIZE - 1);
}

static bool is_empty(work_queue* wq) {
    return wq->size == 0;
}

static bool is_contain(work_queue* wq, char* full_path_name, unsigned path_size, unsigned cache_page_index) {
    for (int i = wq->front; i != wq->rear; i = (i + 1) % MAX_WORKQUEUE_SIZE) {
        if (wq->works[i].cache_page_index == cache_page_index &&
            strncmp(wq->works[i].full_path_name, full_path_name, path_size) == 0 ) {
            return true;
        }
    }
    return false;
}

bool insert_work(work_queue* wq, char* full_path_name, unsigned path_size, unsigned cache_page_index) {
    spinlock_lock(&wq->lock);
    if (is_full(wq) || is_contain(wq, full_path_name, path_size, cache_page_index)) {
        spinlock_unlock(&wq->lock);
        return false;
    }
    strncpy(wq->works[wq->rear].full_path_name, full_path_name, path_size);
    wq->works[wq->rear].full_path_name[path_size] = '\0';
    wq->works[wq->rear].path_size = path_size;
    wq->works[wq->rear].cache_page_index = cache_page_index;
    wq->rear = (wq->rear + 1) % MAX_WORKQUEUE_SIZE;
    wq->size++;
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
    wq->size--;
    spinlock_unlock(&wq->lock);
    return true;
}

bool peak_work(work_queue* wq, char* full_path_name, unsigned* cache_page_index) {
    spinlock_lock(&wq->lock);
    if (is_empty(wq)) {
        spinlock_unlock(&wq->lock);
        return false;
    }
    strncpy(full_path_name, wq->works[wq->front].full_path_name, wq->works[wq->front].path_size);
    full_path_name[wq->works[wq->front].path_size] = '\0';
    *cache_page_index = wq->works[wq->front].cache_page_index;
    spinlock_unlock(&wq->lock);
    return true;
}

void print_work_queue(work_queue* wq) {
    spinlock_lock(&wq->lock);
    printf("front: %u, rear: %u, size: %u\n", wq->front, wq->rear, wq->size);
    for (int i = wq->front; i != wq->rear; i = (i + 1) % MAX_WORKQUEUE_SIZE) {
        printf("%u: full_path_name: %s, cache_page_index: %u\n", i, wq->works[i].full_path_name, wq->works[i].cache_page_index);
    }
    spinlock_unlock(&wq->lock);
}