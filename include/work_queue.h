#ifndef WORK_QUEUE_H
#define WORK_QUEUE_H

/*
 *	Author: Hyam
 *	Date: 2023/03/1
 *	Description: 由users提交work，admin取得work，work內容為HDD to SSD的請求(promotion)，位於work queue中的work不可重複
 *                  解 1 consumer N producer 問題
 */

#include "config.h"
#include "atomic.h"
#include "stdinc.h"

typedef struct {
    char full_path_name[MAX_PATH_SIZE + 1]; // plus '\0'
    unsigned path_size;
    unsigned cache_page_index;
} work;

typedef struct {
    work work_queue[MAX_WORKQUEUE_SIZE];
    int front;
    int rear;
    int size;
    spinlock lock;
} work_queue;

void init_work_queue(work_queue* wq);
bool insert_work(work_queue* wq, char* full_path_name, unsigned path_size, unsigned cache_page_index);  // insert into mru
bool peak_work(work_queue* wq, char* full_path_name, unsigned *cache_page_index);   // get from lru
bool remove_work(work_queue* wq);   // remove peak work





#endif
