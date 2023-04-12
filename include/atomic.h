#ifndef ATOMIC_H
#define ATOMIC_H

/**
 *  @author Hyam
 *  @date 2023/03/17
 *  @brief Spinlock
 */

#include <stdatomic.h>

typedef struct {
    atomic_int locked;
} spinlock;

/**
 * @brief Init lock
 * @param lock - Shared lock
 * @return No return value
 */
void spinlock_init(spinlock* lock);

/**
 * @brief Try to get lock
 * @param lock - Shared lock
 * @return No return value
 */
void spinlock_lock(spinlock* lock);

/**
 * @brief Release lock
 * @param lock - Shared lock
 * @return No return value
 */
void spinlock_unlock(spinlock* lock);

#endif