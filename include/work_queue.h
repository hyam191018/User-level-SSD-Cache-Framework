#ifndef WORK_QUEUE_H
#define WORK_QUEUE_H

/*
 *	Author: Hyam
 *	Date: 2023/03/1
 *	Description: 由users提交work，admin取得work，work內容為HDD to SSD的請求(promotion)
 */

#include "config.h"
#include "atomic.h"
#include "stdinc.h"

typedef struct{    
    char full_path_name[MAX_PATH_SIZE];
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
bool is_empty_work_queue(work_queue* wq);
bool is_full_work_queue(work_queue* wq);
bool push_work(work_queue* wq, char* full_path_name, unsigned *cache_page_index);
bool pop_work(work_queue* wq, char* full_path_name, unsigned *cache_page_index);
bool remove_work(work_queue* wq, char* full_path_name, unsigned *cache_page_index);
bool lookup_work(work_queue* wq, char* full_path_name, unsigned *cache_page_index);





#endif
