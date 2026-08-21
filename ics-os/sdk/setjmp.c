/* i386 setjmp / longjmp for ICS-OS — compilable by TinyCC and gcc -m32. */
#include <setjmp.h>

int setjmp(jmp_buf env)
{
   int r;
   __asm__ volatile (
      "movl %1, %%eax\n\t"
      "movl %%ebx, 0(%%eax)\n\t"
      "movl %%esi, 4(%%eax)\n\t"
      "movl %%edi, 8(%%eax)\n\t"
      "movl %%ebp, 12(%%eax)\n\t"
      "leal 4(%%esp), %%ecx\n\t"
      "movl %%ecx, 16(%%eax)\n\t"
      "movl (%%esp), %%ecx\n\t"
      "movl %%ecx, 20(%%eax)\n\t"
      "xorl %%eax, %%eax\n\t"
      : "=a"(r)
      : "m"(env)
      : "ecx", "memory");
   return r;
}

void longjmp(jmp_buf env, int val)
{
   __asm__ volatile (
      "movl %0, %%edx\n\t"
      "movl %1, %%eax\n\t"
      "testl %%eax, %%eax\n\t"
      "jnz 1f\n\t"
      "movl $1, %%eax\n\t"
      "1:\n\t"
      "movl 0(%%edx), %%ebx\n\t"
      "movl 4(%%edx), %%esi\n\t"
      "movl 8(%%edx), %%edi\n\t"
      "movl 12(%%edx), %%ebp\n\t"
      "movl 16(%%edx), %%esp\n\t"
      "jmp *20(%%edx)\n\t"
      :
      : "m"(env), "g"(val)
      : "memory");
}
