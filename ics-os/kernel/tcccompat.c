/* Helpers so in-OS TinyCC can compile/link the kernel (missing GCC builtins). */

/* TinyCC's internal linker does not consume the kernel GNU linker script.
   This object is linked after the kernel C objects, making this marker the
   upper bound used by startup.S when clearing the combined C BSS. */
char bssEnd[1];

int __sync_fetch_and_add(int *p, int v)
{
   int x;
   __asm__ volatile ("cli");
   x = *p;
   *p = x + v;
   __asm__ volatile ("sti");
   return x;
}

int __sync_lock_test_and_set(int *p, int v)
{
   int x;
   __asm__ volatile ("cli");
   x = *p;
   *p = v;
   __asm__ volatile ("sti");
   return x;
}

void __sync_lock_release(int *p)
{
   *p = 0;
}
