#ifndef ICSOS_IRQ_LIFECYCLE_H
#define ICSOS_IRQ_LIFECYCLE_H

#include "../types.h"

#define IRQ_VECTOR_FIRST_DEVICE 0x42
#define IRQ_VECTOR_LAST_DEVICE  0xEF

typedef struct {
    const void *owner;
    volatile u32 active;
    int releasing;
    int reserved;
} irq_vector_state;

typedef struct {
    const char *name;
    u8 first_vector;
    u8 last_vector;
} irq_domain;

typedef struct {
    u32 address_lo;
    u32 address_hi;
    u32 data;
} irq_msi_message;

static inline int irq_domain_valid(const irq_domain *domain)
{
    return domain && domain->first_vector >= IRQ_VECTOR_FIRST_DEVICE &&
           domain->first_vector <= domain->last_vector &&
           domain->last_vector <= IRQ_VECTOR_LAST_DEVICE;
}

static inline int irq_domain_compose_xapic_msi(const irq_domain *domain,
                                               u8 vector, u32 destination,
                                               irq_msi_message *message)
{
    if (!irq_domain_valid(domain) || !message ||
        vector < domain->first_vector || vector > domain->last_vector ||
        destination > 0xFF)
        return 0;
    message->address_lo = 0xFEE00000u | (destination << 12);
    message->address_hi = 0;
    message->data = vector;
    return 1;
}

static inline int irq_vector_state_claim(irq_vector_state *state,
                                         const void *owner)
{
    if (!state || !owner || state->owner)
        return 0;
    state->owner = owner;
    state->active = 0;
    state->releasing = 0;
    state->reserved = 0;
    return 1;
}

static inline int irq_vector_state_reserve(irq_vector_state *state,
                                           const void *owner)
{
    if (!state || !owner || state->owner)
        return 0;
    state->owner = owner;
    state->active = 0;
    state->releasing = 0;
    state->reserved = 1;
    return 1;
}

static inline int irq_vector_state_unreserve(irq_vector_state *state,
                                             const void *owner)
{
    if (!state || state->owner != owner || !state->reserved ||
        state->active || state->releasing)
        return 0;
    state->owner = 0;
    state->reserved = 0;
    return 1;
}

static inline int irq_vector_state_enter(irq_vector_state *state,
                                         const void *owner)
{
    if (!state || state->owner != owner || state->releasing || state->reserved)
        return 0;
    state->active++;
    return 1;
}

static inline int irq_vector_state_exit(irq_vector_state *state,
                                        const void *owner)
{
    if (!state || state->owner != owner || !state->active)
        return 0;
    state->active--;
    return 1;
}

static inline int irq_vector_state_begin_release(irq_vector_state *state,
                                                 const void *owner)
{
    if (!state || state->owner != owner || state->releasing || state->reserved)
        return 0;
    state->releasing = 1;
    return 1;
}

static inline int irq_vector_state_finish_release(irq_vector_state *state,
                                                  const void *owner)
{
    if (!state || state->owner != owner || !state->releasing || state->active)
        return 0;
    state->owner = 0;
    state->releasing = 0;
    return 1;
}

static inline int irq_vector_state_allocate(irq_vector_state *states,
                                            u32 count, u8 first, u8 last,
                                            const void *owner, u8 *vector)
{
    u32 current;
    if (!states || !owner || !vector || first > last || last >= count)
        return 0;
    for (current = first; current <= last; current++) {
        if (irq_vector_state_claim(&states[current], owner)) {
            *vector = (u8)current;
            return 1;
        }
    }
    return 0;
}

int irq_vector_claim(u8 vector, const void *owner);
int irq_vector_allocate(const void *owner, u8 *vector);
int irq_domain_allocate(const irq_domain *domain, const void *owner,
                        u8 *vector);
int irq_vector_reserve(u8 vector, const void *owner);
int irq_vector_unreserve(u8 vector, const void *owner);
int irq_vector_enter(u8 vector, const void *owner);
void irq_vector_exit(u8 vector, const void *owner);
int irq_vector_release(u8 vector, const void *owner);

extern const irq_domain irq_device_domain;

#endif