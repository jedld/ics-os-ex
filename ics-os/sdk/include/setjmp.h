#ifndef _SETJMP_H
#define _SETJMP_H

#ifdef __x86_64__
/* rbx,rbp,r12,r13,r14,r15,rsp,rip */
typedef unsigned long jmp_buf[8];
#else
/* ebx,esi,edi,ebp,esp,eip */
typedef unsigned long jmp_buf[6];
#endif

int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val);

#endif
