#ifndef ATOMIC_H
#define ATOMIC_H

/*
 * Author: Hyam
 * Date: 2023/03/17
 * Description: 使用 spinlock
 */

#include <stdatomic.h>

typedef struct {
    atomic_int locked;
} spinlock;

/* in atomic.c */
void spinlock_init(spinlock* lock);
void spinlock_lock(spinlock* lock);
void spinlock_unlock(spinlock* lock);

#endif