#include "atomic.h"

static inline int atomic_compare_exchange(atomic_uint* ptr, unsigned int compare,
                                          unsigned int exchange) {
    return atomic_compare_exchange_strong(ptr, &compare, exchange);
}

static inline unsigned int atomic_add_fetch(atomic_uint* ptr, unsigned int d) {
    return atomic_fetch_add_explicit(ptr, d, memory_order_seq_cst);
}

void spinlock_init(spinlock* lock) {
    atomic_store_explicit(&(lock->locked), 0, memory_order_seq_cst);
}

void spinlock_lock(spinlock* lock) {
    while (!atomic_compare_exchange(&(lock->locked), 0, 1)) {
    }
}

void spinlock_unlock(spinlock* lock) {
    atomic_store_explicit(&(lock->locked), 0, memory_order_seq_cst);
}