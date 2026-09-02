#include <stdio.h>

/* blkcache.h's public API uses the legacy DWORD spelling. */
typedef unsigned int DWORD;
#include "kernel/iomgr/blkcache.h"
#include "kernel/devmgr/devmgr_lifecycle.h"
#include "kernel/process/completion.h"

static int reschedule_count;

void smp_reschedule_others(void)
{
   reschedule_count++;
}

void wait_queue_wake_all(wait_queue_t *queue)
{
   (void)queue;
}

void wait_event_wake_all(void *key)
{
   (void)key;
}

static int check(const char *name, int condition)
{
   if (!condition) {
      printf("not ok - %s\n", name);
      return 0;
   }
   printf("ok - %s\n", name);
   return 1;
}

int main(void)
{
   int ok = 1;
   DWORD refs = 0;
   completion_t completion;

   printf("TAP version 13\n1..12\n");
   ok &= check("matching writeback generation clears dirty",
               blkcache_writeback_is_current(1, 7, 7));
   ok &= check("redirty during writeback remains dirty",
               !blkcache_writeback_is_current(1, 8, 7));
   ok &= check("reused cache slot remains dirty",
               !blkcache_writeback_is_current(0, 7, 7));
   ok &= check("same device generation retains cache identity",
               blkcache_device_key_is_current(4, 4));
   ok &= check("reused device slot rejects stale cache identity",
               !blkcache_device_key_is_current(4, 5));
   ok &= check("live device reference is acquired",
               devmgr_lifecycle_get(DEVMGR_STATE_LIVE, &refs) && refs == 1);
   ok &= check("quiescing device rejects new references",
               !devmgr_lifecycle_get(DEVMGR_STATE_QUIESCING, &refs) && refs == 1);
   ok &= check("quiescing device waits for active reference",
               !devmgr_lifecycle_can_retire(DEVMGR_STATE_QUIESCING, refs));
   ok &= check("active reference is released",
               devmgr_lifecycle_put(&refs) && refs == 0);
   ok &= check("quiescing device retires after drain",
               devmgr_lifecycle_can_retire(DEVMGR_STATE_QUIESCING, refs) &&
               !devmgr_lifecycle_put(&refs));
   completion_init(&completion);
   ok &= check("new completion is not signaled",
               !completion_done(&completion));
   complete_all(&completion);
   ok &= check("completion-before-wait is retained",
               completion_done(&completion) && reschedule_count == 1);
   return ok ? 0 : 1;
}
