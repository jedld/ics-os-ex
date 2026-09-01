#ifndef ICSOS_SPINLOCK_H
#define ICSOS_SPINLOCK_H

#include "../types.h"

typedef struct {
    volatile u32 locked;
} spinlock_t;

typedef u64 spin_irq_flags_t;

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

/* Interrupt masking prevents same-CPU IRQ recursion; the spinlock provides
 * exclusion against all other CPUs. */
static inline spin_irq_flags_t spin_lock_irqsave(spinlock_t *l) {
    spin_irq_flags_t flags;
    __asm__ __volatile__("pushfq; popq %0; cli" : "=r"(flags) : : "memory");
    spin_lock(l);
    return flags;
}

static inline void spin_unlock_irqrestore(spinlock_t *l,
                                           spin_irq_flags_t flags) {
    spin_unlock(l);
    if (flags & (1ULL << 9))
        __asm__ __volatile__("sti" : : : "memory");
}

#endif
