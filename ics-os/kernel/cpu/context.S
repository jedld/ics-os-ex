/* Software context switch for x86_64 (kernel threads). */
.text
.globl context_switch
.globl context_load
.globl fpu_save
.globl fpu_restore

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
    movq %rax, CTX_RFLAGS(%rdi)
    movq %cr3, %rax
    movq %rax, CTX_CR3(%rdi)

    /* Old task is fully saved — allow another CPU to pick it up. */
    testq %rdx, %rdx
    jz 3f
    movl $-1, (%rdx)
3:
    movq %rsi, %rdi

context_load:
    movq CTX_CR3(%rdi), %rax
    testq %rax, %rax
    jz 1f
    movq %cr3, %rcx
    cmpq %rax, %rcx
    je 1f
    movq %rax, %cr3
1:
    movq CTX_RSP(%rdi), %rsp
    movq CTX_R15(%rdi), %r15
    movq CTX_R14(%rdi), %r14
    movq CTX_R13(%rdi), %r13
    movq CTX_R12(%rdi), %r12
    movq CTX_RBP(%rdi), %rbp
    movq CTX_RBX(%rdi), %rbx
    movq CTX_RFLAGS(%rdi), %rax
    testq %rax, %rax
    jz 2f
    pushq %rax
    popfq
2:
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
