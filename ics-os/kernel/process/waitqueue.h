#ifndef ICSOS_WAITQUEUE_H
#define ICSOS_WAITQUEUE_H

#include "../types.h"
#include "../cpu/spinlock.h"

struct _PCB386;

typedef struct wait_queue {
   spinlock_t lock;
   struct _PCB386 *head;
} wait_queue_t;

static inline void wait_queue_init(wait_queue_t *queue)
{
   spin_init(&queue->lock);
   queue->head=0;
}

void wait_queue_prepare(wait_queue_t *queue, void *key, u32 deadline);
void wait_queue_finish(wait_queue_t *queue);
void wait_queue_wake_all(wait_queue_t *queue);
void wait_queue_cancel(struct _PCB386 *process);
void wait_event_wake_all(void *key);

#endif
