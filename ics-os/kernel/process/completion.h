#ifndef ICSOS_COMPLETION_H
#define ICSOS_COMPLETION_H

#include "../dextypes.h"
#include "../cpu/smp.h"
#include "waitqueue.h"

/* One-shot IRQ-safe completion. The condition is authoritative; the reschedule
   IPI is only a latency hint, so completion before wait cannot be lost. */
typedef struct {
   volatile DWORD done;
} completion_t;

static inline void completion_init(completion_t *completion)
{
   __atomic_store_n(&completion->done, 0, __ATOMIC_RELAXED);
}

static inline void reinit_completion(completion_t *completion)
{
   __atomic_store_n(&completion->done, 0, __ATOMIC_RELAXED);
}

static inline int completion_done(const completion_t *completion)
{
   return __atomic_load_n(&completion->done, __ATOMIC_ACQUIRE) != 0;
}

static inline void complete_all(completion_t *completion)
{
   __atomic_store_n(&completion->done, 1, __ATOMIC_RELEASE);
   wait_event_wake_all(completion);
   smp_reschedule_others();
}

int wait_for_completion_until(completion_t *completion, DWORD deadline);

#endif
