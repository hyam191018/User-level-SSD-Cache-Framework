#ifndef WORK_QUEUE_H
#define WORK_QUEUE_H

/**
 *  @author Hyam
 *  @date 2023/03/21
 *  @brief 由users提交work，admin取得work，work queue中的work不可重複
 *                  目前是使用循環陣列，導致有效能瓶頸，應該能設計更好的演算法
 *                  (解 1 consumer N producer 問題)
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
 * @brief Init work queue, queue size default is cblock number / 2
 *              Return:  No return value
 * @return No return value
 */
void init_work_queue(work_queue* wq);

/**
 * @brief Check work queue state
 * @return true, if work queue is full
 *         false, if not
 */
bool is_full(work_queue* wq);

/**
 * @brief Check work queue state
 * @return true, if work queue is empty
 *         false, if not
 */
bool is_empty(work_queue* wq);

/**
 * @brief Push a promote request to work queue
 * @param full_path_name - key
 * @param cache_page_index - key
 * @return true, if success
 *         false, if queue is full or work is contain in queue
 */
bool insert_work(work_queue* wq, char* full_path_name, unsigned cache_page_index);

/**
 * @brief Search a work
 * @param full_path_name - key
 * @param cache_page_index - key
 * @return true, if work in queue
 *         false, if not
 */
bool contains_work(work_queue* wq, char* full_path_name, unsigned cache_page_index);

/**
 * @brief Get a promote request from work queue
 * @param full_path_name - file for promote
 * @param cache_page_index - offset of the file
 * @return true, if success
 *         false, if queue is empty
 */
bool peak_work(work_queue* wq, char* full_path_name, unsigned* cache_page_index);

/**
 * @brief Remove peak work from the queue
 * @return true, if success
 *         false, if queue is empty
 */
bool remove_work(work_queue* wq);

#endif
