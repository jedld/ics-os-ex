/* setjmp / longjmp for ICS-OS — compilable by TinyCC and host gcc. */
#include <setjmp.h>

#ifdef __x86_64__
int setjmp(jmp_buf env)
{
   long r;
   __asm__ volatile (
      "movq %1, %%rax\n\t"
      "movq %%rbx, 0(%%rax)\n\t"
      "movq %%rbp, 8(%%rax)\n\t"
      "movq %%r12, 16(%%rax)\n\t"
      "movq %%r13, 24(%%rax)\n\t"
      "movq %%r14, 32(%%rax)\n\t"
      "movq %%r15, 40(%%rax)\n\t"
      "leaq 8(%%rsp), %%rcx\n\t"
      "movq %%rcx, 48(%%rax)\n\t"
      "movq (%%rsp), %%rcx\n\t"
      "movq %%rcx, 56(%%rax)\n\t"
      "xorq %%rax, %%rax\n\t"
      : "=a"(r)
      : "m"(env)
      : "rcx", "memory");
   return (int)r;
}

void longjmp(jmp_buf env, int val)
{
   __asm__ volatile (
      "movq %0, %%rdx\n\t"
      "movl %1, %%eax\n\t"
      "testl %%eax, %%eax\n\t"
      "jnz 1f\n\t"
      "movl $1, %%eax\n\t"
      "1:\n\t"
      "movq 0(%%rdx), %%rbx\n\t"
      "movq 8(%%rdx), %%rbp\n\t"
      "movq 16(%%rdx), %%r12\n\t"
      "movq 24(%%rdx), %%r13\n\t"
      "movq 32(%%rdx), %%r14\n\t"
      "movq 40(%%rdx), %%r15\n\t"
      "movq 48(%%rdx), %%rsp\n\t"
      "jmpq *56(%%rdx)\n\t"
      :
      : "m"(env), "g"(val)
      : "memory");
}
#else
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
#endif
