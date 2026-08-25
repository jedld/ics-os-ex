/* 64-bit interrupt / exception wrappers — System V AMD64. */
.text

.extern time_handler
.extern kbd_irq
.extern mouse_irq
.extern pagefaulthandler
.extern fdchandler
.extern api_syscall
.extern GPFhandler
.extern GPFhandler64
.extern CPUint
.extern nocoprocessor
.extern divide_error
.extern exc_invalidtss
.extern irq_activate
.extern schedule_from_timer
.extern lapic_eoi
.extern smp_reschedule_ipi

.macro PUSH_ALL
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
.endm

.macro POP_ALL
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax
.endm

.globl syscallwrapper
syscallwrapper:
    PUSH_ALL
    /* After PUSH_ALL, stack top is r15..rax (rax at offset 14*8=112).
       Legacy DEX int 0x30 ABI: rax=fxn, rbx=a1, rcx=a2, rdx=a3, rsi=a4, rdi=a5. */
    movq 112(%rsp), %rdi        /* saved rax -> fxn */
    movq 104(%rsp), %rsi        /* saved rbx -> a1 */
    movq 96(%rsp), %rdx         /* saved rcx -> a2 */
    movq 88(%rsp), %rcx         /* saved rdx -> a3 */
    movq 80(%rsp), %r8          /* saved rsi -> a4 */
    movq 72(%rsp), %r9          /* saved rdi -> a5 */
    call api_syscall
    movq %rax, 112(%rsp)        /* return value in caller's rax */
    POP_ALL
    iretq

.globl CPUintwrapper
CPUintwrapper:
    cli
    PUSH_ALL
    call CPUint
    POP_ALL
    iretq

.globl fdcwrapper
fdcwrapper:
    cli
    PUSH_ALL
    call fdchandler
    POP_ALL
    iretq

.globl copwrapper
copwrapper:
    cli
    PUSH_ALL
    call nocoprocessor
    POP_ALL
    iretq

.globl timerwrapper
timerwrapper:
    cli
    PUSH_ALL
    call time_handler
    call schedule_from_timer
    POP_ALL
    iretq

.globl reschedwrapper
reschedwrapper:
    cli
    PUSH_ALL
    call smp_reschedule_ipi
    POP_ALL
    iretq

.globl kbdwrapper
kbdwrapper:
    PUSH_ALL
    call kbd_irq
    POP_ALL
    iretq

.globl mousewrapper
mousewrapper:
    PUSH_ALL
    call mouse_irq
    POP_ALL
    iretq

.globl gpfwrapper
gpfwrapper:
    cli
    /* On entry: 0(rsp)=error, 8(rsp)=rip, 16(rsp)=cs, 24(rsp)=rflags */
    PUSH_ALL
    /* After PUSH_ALL (15 regs): error at 120, rip at 128, cs at 136 */
    /* struct gpf_info layout (gcc): rip(0) cr2(8) err(16) pad(20) cs(20) ss(24)
       rsp(32) cr3(40) sizeof=48 */
    subq $64, %rsp
    movq 192(%rsp), %rax        /* saved rip (128 + 64) */
    movq %rax, 0(%rsp)
    movq %cr2, %rax
    movq %rax, 8(%rsp)
    movl 184(%rsp), %eax        /* error code (120 + 64) */
    movl %eax, 16(%rsp)
    movw 200(%rsp), %ax         /* cs (136 + 64) -> field cs @20 */
    movw %ax, 20(%rsp)
    movw %ss, %ax
    movw %ax, 24(%rsp)
    leaq 8(%rsp), %rax          /* active RSP = return frame (rip+8) */
    movq %rax, 32(%rsp)
    movq %cr3, %rax
    movq %rax, 40(%rsp)
    movq %rsp, %rdi
    call GPFhandler64
    addq $64, %rsp
    POP_ALL
    addq $8, %rsp
    iretq

.globl pfwrapper
pfwrapper:
    cli
    /* On entry: 0(rsp)=error, 8(rsp)=rip, ... */
    PUSH_ALL
    movq 128(%rsp), %rdi        /* faulting RIP / CR2 handled in C? pass rip */
    movq 120(%rsp), %rsi        /* error code as fault_info approx */
    /* Prefer CR2 for location — pagefaulthandler(location, fault_info) */
    movq %cr2, %rdi
    movq 120(%rsp), %rsi
    call pagefaulthandler
    POP_ALL
    addq $8, %rsp
    iretq

.globl div_wrapper
div_wrapper:
    cli
    PUSH_ALL
    call divide_error
    POP_ALL
    iretq

.globl invalidtsswrapper
invalidtsswrapper:
    cli
    PUSH_ALL
    call exc_invalidtss
    POP_ALL
    addq $8, %rsp
    iretq

.globl irq1wrapper
irq1wrapper:
    PUSH_ALL
    movq $1, %rdi
    call irq_activate
    POP_ALL
    iretq

.globl irq2wrapper
irq2wrapper:
    PUSH_ALL
    movq $2, %rdi
    call irq_activate
    POP_ALL
    iretq

.globl irq3wrapper
irq3wrapper:
    PUSH_ALL
    movq $3, %rdi
    call irq_activate
    POP_ALL
    iretq

.globl irq4wrapper
irq4wrapper:
    PUSH_ALL
    movq $4, %rdi
    call irq_activate
    POP_ALL
    iretq

.globl irq5wrapper
irq5wrapper:
    PUSH_ALL
    movq $5, %rdi
    call irq_activate
    POP_ALL
    iretq

.globl irq6wrapper
irq6wrapper:
    PUSH_ALL
    movq $6, %rdi
    call irq_activate
    POP_ALL
    iretq

.globl irq7wrapper
irq7wrapper:
    PUSH_ALL
    movq $7, %rdi
    call irq_activate
    POP_ALL
    iretq

.globl irq8wrapper
irq8wrapper:
    PUSH_ALL
    movq $8, %rdi
    call irq_activate
    POP_ALL
    iretq

.globl irq9wrapper
irq9wrapper:
    PUSH_ALL
    movq $9, %rdi
    call irq_activate
    POP_ALL
    iretq

.globl irq10wrapper
irq10wrapper:
    PUSH_ALL
    movq $10, %rdi
    call irq_activate
    POP_ALL
    iretq

.globl irq11wrapper
irq11wrapper:
    PUSH_ALL
    movq $11, %rdi
    call irq_activate
    POP_ALL
    iretq

.globl irq12wrapper
irq12wrapper:
    PUSH_ALL
    movq $12, %rdi
    call irq_activate
    POP_ALL
    iretq
