/* Shim providing the i386 GGC allocators expected by i386.c.
   In the standard build these resolve via GGC machinery; here we provide
   concrete definitions that allocate zeroed GC memory of the correct size. */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "ggc.h"
#include "tm.h"

/* Mirror of the file-local struct stack_local_entry in i386.c, used only
   to size the allocation (its fields must match i386.c exactly). */
struct shim_stack_local_entry {
  unsigned short mode;
  unsigned short n;
  rtx rtl;
  struct shim_stack_local_entry *next;
};

struct machine_function *
ggc_alloc_cleared_machine_function (void)
{
  return (struct machine_function *)
    ggc_internal_cleared_alloc_stat (sizeof (struct machine_function)
                                     MEM_STAT_INFO);
}

struct stack_local_entry *
ggc_alloc_stack_local_entry (void)
{
  return (struct stack_local_entry *)
    ggc_internal_cleared_alloc_stat (sizeof (struct shim_stack_local_entry)
                                     MEM_STAT_INFO);
}

/* gengtype ran without the i386 machine_function type, so gtype-desc.c
   emits `gcc_assert (!cfun->machine)` instead of walking it.  On a host
   gcc ≥ 4.5 that assert is __builtin_unreachable when machine is set.
   Walk the i386 GC pointers ourselves. */
void
gt_ggc_mx_machine_function (void *x_p)
{
  struct machine_function *x = (struct machine_function *)x_p;
  struct shim_stack_local_entry *s;

  if (!x)
    return;
  if (!ggc_test_and_set_mark (x))
    return;

  s = (struct shim_stack_local_entry *)x->stack_locals;
  while (s)
    {
      struct shim_stack_local_entry *next = s->next;
      if (ggc_test_and_set_mark (s) && s->rtl)
        gt_ggc_mx_rtx_def (s->rtl);
      s = next;
    }
  if (x->split_stack_varargs_pointer)
    gt_ggc_mx_rtx_def (x->split_stack_varargs_pointer);
}

/* c-common.c calls mudflap_init() under a runtime `if (flag_mudflap)` guard.
   Mudflap is not built here, so provide a no-op to satisfy the link. */
void
mudflap_init (void)
{
}
