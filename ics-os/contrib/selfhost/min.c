/* Minimal in-OS compiler test: no headers, no libc. */
void _start(void)
{
   /* exit(0) via int 0x30 / FXN_EXIT=3 (legacy DEX regs). */
   __asm__ __volatile__(
      "mov $3, %%eax\n\t"
      "xor %%ebx, %%ebx\n\t"
      "xor %%ecx, %%ecx\n\t"
      "xor %%edx, %%edx\n\t"
      "int $0x30"
      :
      :
      : "rax", "rbx", "rcx", "rdx", "memory");
}
