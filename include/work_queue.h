#ifndef WORK_QUEUE_H
#define WORK_QUEUE_H

/**
 *  Author: Hyam
 *  Date: 2023/03/21
 *  Description: 由users提交work，admin取得work，work queue中的work不可重複
 *              (解 1 consumer N producer 問題)
 */

#include "atomic.h"
#include "config.h"
#include "stdinc.h"

typedef struct {
    char full_path_name[MAX_PATH_SIZE + 1];
    unsigned cache_page_index;
} work;

typedef struct {
    work works[MAX_WORKQUEUE_SIZE];
    int front;
    int rear;
    int size;
    spinlock lock;
} work_queue;

/**
 * Description: Init work queue, queue size default is cblock number / 2
 *              Return:  No return value
 */
void init_work_queue(work_queue* wq);

/**
 * Description: Check work queue state
 * Return:  true, if work queue is full
 *          false, if not
 */
bool is_full(work_queue* wq);

/**
 * Description: Check work queue state
 * Return:  true, if work queue is empty
 *          false, if not
 */
bool is_empty(work_queue* wq);

/**
 * Description: Push a promote request to work queue
 * Return:  true, if success
 *          false, if queue is full or work is contain in queue
 */
bool insert_work(work_queue* wq, char* full_path_name, unsigned cache_page_index);

/**
 * Description: Search a work
 * Return:  true, if work in queue
 *          false, if not
 */
bool contains_work(work_queue* wq, char* full_path_name, unsigned cache_page_index);

/**
 * Description: Get a promote request from work queue
 * Return:  true, if success
 *          false, if queue is empty
 */
bool peak_work(work_queue* wq, char* full_path_name, unsigned* cache_page_index);

/**
 * Description: Remove peak work from the queue
 * Return:  true, if success
 *          false, if queue is empty
 */
bool remove_work(work_queue* wq);

#endif
