/* Software context switch for x86_64 (kernel threads). */
.text
.globl context_switch
.globl context_load
.globl fpu_save
.globl fpu_restore
.globl ctx_load_in_progress

.set CTX_R15, 0
.set CTX_R14, 8
.set CTX_R13, 16
.set CTX_R12, 24
.set CTX_R11, 32
.set CTX_R10, 40
.set CTX_R9,  48
.set CTX_R8,  56
.set CTX_RBP, 64
.set CTX_RDI, 72
.set CTX_RSI, 80
.set CTX_RDX, 88
.set CTX_RCX, 96
.set CTX_RBX, 104
.set CTX_RAX, 112
.set CTX_RIP, 120
.set CTX_CS,  128
.set CTX_RFLAGS, 136
.set CTX_RSP, 144
.set CTX_SS,  152
.set CTX_CR3, 160

context_switch:
    /* rdi=old, rsi=new, rdx=&old->on_cpu (or NULL) — save then release claim */
    movq %r15, CTX_R15(%rdi)
    movq %r14, CTX_R14(%rdi)
    movq %r13, CTX_R13(%rdi)
    movq %r12, CTX_R12(%rdi)
    movq %rbp, CTX_RBP(%rdi)
    movq %rbx, CTX_RBX(%rdi)
    movq (%rsp), %rax
    movq %rax, CTX_RIP(%rdi)
    leaq 8(%rsp), %rax
    movq %rax, CTX_RSP(%rdi)
    pushfq
    popq %rax
    /* Force IF=1: context_switch runs under the timer's cli (IF=0), so the
       raw RFLAGS would otherwise be saved with IF=0 and the task would later
       resume with interrupts disabled, freezing the timer/scheduler. A
       scheduled task must always resume interruptible. */
    orl $0x200, %eax
    movq %rax, CTX_RFLAGS(%rdi)
    movq %cr3, %rax
    movq %rax, CTX_CR3(%rdi)

    /* Old task is fully saved — allow another CPU to pick it up. */
    testq %rdx, %rdx
    jz 3f
    movl $-1, (%rdx)
3:
    movq %rsi, %rdi

/* Ring-3 note: CTX_CS/SS are saved for a future iretq path. Today we
   ret to kernel CS; user processes record USER_CODE in PCB.usercs. */
/* Ring-3: CTX_CS/SS saved for a future iretq. Today we `ret` in kernel CS. */
context_load:
    movq CTX_CR3(%rdi), %rax
    testq %rax, %rax
    jz 1f
    /* Always reload CR3 to flush TLB when entering a task. */
    movq %rax, %cr3
1:
    movq CTX_RSP(%rdi), %rsp
    /* Set the in-context-load flag NOW (RSP is the new task's).
       A timer IRQ that fires from here until the flag is cleared
       will see ctx_load_in_progress==1 and bail out of
       schedule_from_timer without calling ps_switchto. */
    movl $1, ctx_load_in_progress
    movq CTX_R15(%rdi), %r15
    movq CTX_R14(%rdi), %r14
    movq CTX_R13(%rdi), %r13
    movq CTX_R12(%rdi), %r12
    movq CTX_RBP(%rdi), %rbp
    movq CTX_RBX(%rdi), %rbx
    movq CTX_RFLAGS(%rdi), %rax
    testq %rax, %rax
    jz 2f
    /* Force IF=1 on resume as well (belt-and-suspenders for any context
       saved with IF=0 before the save-side fix, and for the first switch of
       a freshly-created task). */
    orl $0x200, %eax
    pushq %rax
    popfq
    2:
    /* The new task's context is fully loaded.  We must clear
       ctx_load_in_progress so that the NEXT timer tick can
       schedule.  But we cannot clear it here and then ret: a timer
       IRQ in the window between the clear and the ret would run
       schedule_from_timer, see ctx_load_in_progress==0, and call
       ps_switchto — which would save the new task's live RIP
       (inside context_load) over its seeded entry point.

       Instead, clear the flag from the new task's first timer tick:
       schedule_from_timer checks and clears ctx_load_in_progress
       before proceeding.  This is safe because the new task is
       already running (RIP is about to be set to the entry point). */
   movl $0, ctx_load_in_progress
   movq CTX_RIP(%rdi), %rax
   pushq %rax
   ret

fpu_save:
    /* rdi may be unaligned inside PCB — copy via aligned scratch */
    pushq %rdi
    leaq fpu_scratch(%rip), %rdi
    fxsave (%rdi)
    popq %rdi
    pushq %rsi
    pushq %rdi
    leaq fpu_scratch(%rip), %rsi
    movl $512, %ecx
    rep movsb
    popq %rdi
    popq %rsi
    ret

fpu_restore:
    pushq %rsi
    pushq %rdi
    movq %rdi, %rsi
    leaq fpu_scratch(%rip), %rdi
    movl $512, %ecx
    rep movsb
    leaq fpu_scratch(%rip), %rdi
    fxrstor (%rdi)
    popq %rdi
    popq %rsi
    ret

.bss
.align 16
fpu_scratch:
    .space 512

/* Set while context_load is between "RSP switched" and "ret".
   schedule_from_timer checks this and bails out to avoid a nested
   ps_switchto that would clobber the in-flight task's saved ctx. */
.align 4
ctx_load_in_progress:
    .long 0
