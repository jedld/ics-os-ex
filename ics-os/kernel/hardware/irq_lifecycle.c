#include "irq_lifecycle.h"
#include "../cpu/spinlock.h"

#define IRQ_RELEASE_SPINS       4000000

static irq_vector_state irq_vectors[256];
static spinlock_t irq_vector_lock;

const irq_domain irq_device_domain = {
    "device",
    IRQ_VECTOR_FIRST_DEVICE,
    IRQ_VECTOR_LAST_DEVICE
};

static int irq_vector_valid(u8 vector)
{
    return vector >= IRQ_VECTOR_FIRST_DEVICE &&
           vector <= IRQ_VECTOR_LAST_DEVICE;
}

int irq_vector_claim(u8 vector, const void *owner)
{
    spin_irq_flags_t flags;
    int claimed;
    if (!irq_vector_valid(vector))
        return 0;
    flags = spin_lock_irqsave(&irq_vector_lock);
    claimed = irq_vector_state_claim(&irq_vectors[vector], owner);
    spin_unlock_irqrestore(&irq_vector_lock, flags);
    return claimed;
}

int irq_vector_allocate(const void *owner, u8 *vector)
{
    return irq_domain_allocate(&irq_device_domain, owner, vector);
}

int irq_domain_allocate(const irq_domain *domain, const void *owner,
                        u8 *vector)
{
    spin_irq_flags_t flags;
    int claimed;
    if (!irq_domain_valid(domain))
        return 0;
    flags = spin_lock_irqsave(&irq_vector_lock);
    claimed = irq_vector_state_allocate(irq_vectors, 256,
                                        domain->first_vector,
                                        domain->last_vector,
                                        owner, vector);
    spin_unlock_irqrestore(&irq_vector_lock, flags);
    return claimed;
}

int irq_vector_reserve(u8 vector, const void *owner)
{
    spin_irq_flags_t flags;
    int reserved;
    if (!irq_vector_valid(vector))
        return 0;
    flags = spin_lock_irqsave(&irq_vector_lock);
    reserved = irq_vector_state_reserve(&irq_vectors[vector], owner);
    spin_unlock_irqrestore(&irq_vector_lock, flags);
    return reserved;
}

int irq_vector_unreserve(u8 vector, const void *owner)
{
    spin_irq_flags_t flags;
    int released;
    if (!irq_vector_valid(vector))
        return 0;
    flags = spin_lock_irqsave(&irq_vector_lock);
    released = irq_vector_state_unreserve(&irq_vectors[vector], owner);
    spin_unlock_irqrestore(&irq_vector_lock, flags);
    return released;
}

int irq_vector_enter(u8 vector, const void *owner)
{
    int entered;
    if (!irq_vector_valid(vector))
        return 0;
    spin_lock(&irq_vector_lock);
    entered = irq_vector_state_enter(&irq_vectors[vector], owner);
    spin_unlock(&irq_vector_lock);
    return entered;
}

void irq_vector_exit(u8 vector, const void *owner)
{
    if (!irq_vector_valid(vector))
        return;
    spin_lock(&irq_vector_lock);
    (void)irq_vector_state_exit(&irq_vectors[vector], owner);
    spin_unlock(&irq_vector_lock);
}

int irq_vector_release(u8 vector, const void *owner)
{
    spin_irq_flags_t flags;
    u32 spins;
    int released;
    if (!irq_vector_valid(vector))
        return 0;
    flags = spin_lock_irqsave(&irq_vector_lock);
    if (!irq_vector_state_begin_release(&irq_vectors[vector], owner)) {
        spin_unlock_irqrestore(&irq_vector_lock, flags);
        return 0;
    }
    spin_unlock_irqrestore(&irq_vector_lock, flags);
    for (spins = 0; spins < IRQ_RELEASE_SPINS; spins++) {
        if (!irq_vectors[vector].active)
            break;
        __asm__ __volatile__("pause");
    }
    flags = spin_lock_irqsave(&irq_vector_lock);
    released = irq_vector_state_finish_release(&irq_vectors[vector], owner);
    spin_unlock_irqrestore(&irq_vector_lock, flags);
    return released;
}