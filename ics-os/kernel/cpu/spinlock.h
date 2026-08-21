#ifndef ICSOS_SPINLOCK_H
#define ICSOS_SPINLOCK_H

#include "../types.h"

typedef struct {
    volatile u32 locked;
} spinlock_t;

static inline void spin_init(spinlock_t *l) {
    l->locked = 0;
}

static inline void spin_lock(spinlock_t *l) {
    while (__sync_lock_test_and_set(&l->locked, 1)) {
        while (l->locked)
            __asm__ __volatile__("pause");
    }
}

static inline void spin_unlock(spinlock_t *l) {
    __sync_lock_release(&l->locked);
}

#endif
