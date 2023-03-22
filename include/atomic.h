#ifndef ATOMIC_H
#define ATOMIC_H

/*
 *  Author: Hyam
 *  Date: 2023/03/17
 *  Description: 使用 spinlock
 */

#include <stdatomic.h>

typedef struct {
    atomic_int locked;
} spinlock;

/*
 * Description: init lock to 0
 * Return:  No return value
 */
void spinlock_init(spinlock* lock);

/*
 * Description: try to get lock and set it to 1
 * Return:  No return value
 */
void spinlock_lock(spinlock* lock);

/*
 * Description: release lock to 0
 * Return:  No return value
 */
void spinlock_unlock(spinlock* lock);

#endif