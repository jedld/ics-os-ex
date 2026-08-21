/* Minimal in-OS compiler test: no headers, no libc. */
void _start(void)
{
   /* exit(0) via int 0x30 / FXN_EXIT=3 */
   __asm__ __volatile__(
      "movl $3, %%eax\n\t"
      "xorl %%ebx, %%ebx\n\t"
      "xorl %%ecx, %%ecx\n\t"
      "xorl %%edx, %%edx\n\t"
      "int $0x30"
      :
      :
      : "eax", "ebx", "ecx", "edx", "memory");
}
