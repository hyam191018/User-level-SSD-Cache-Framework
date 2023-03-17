#ifndef ATOMIC_H
#define ATOMIC_H

/*
 * Author: Hyam
 * Date: 2023/03/17
 * Description: 使用spinlock
 */

#include <stdatomic.h>

typedef struct {
    atomic_int locked; // 使用 atomic_int 定義 locked 變數
} spinlock;

/* in atomic.c */
void spinlock_lock(spinlock* lock);
void spinlock_unlock(spinlock* lock);


#endif