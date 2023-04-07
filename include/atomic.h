#ifndef ATOMIC_H
#define ATOMIC_H

/**
 *  Author: Hyam
 *  Date: 2023/03/17
 *  Description: Spinlock
 */

#include <stdatomic.h>

typedef struct {
    atomic_int locked;
} spinlock;

/**
 * Description: Init lock
 * Return:  No return value
 */
void spinlock_init(spinlock* lock);

/**
 * Description: Try to get lock
 * Return:  No return value
 */
void spinlock_lock(spinlock* lock);

/**
 * Description: Release lock
 * Return:  No return value
 */
void spinlock_unlock(spinlock* lock);

#endif