/* Helpers so in-OS TinyCC can compile/link the kernel (missing GCC builtins). */
void stopints(void)
{
   __asm__ volatile ("cli");
}

void startints(void)
{
   __asm__ volatile ("sti");
}

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
