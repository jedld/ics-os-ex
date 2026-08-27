/* kexec trampoline: copied to 0x80000 so it survives overwriting the
   running kernel at 0x100000. Entered in long mode with:
     rdi = staging source
     rsi = byte count
     rdx = new kernel entry (32-bit startup)
     rcx = Multiboot2 info physical address
   Copies the image to 0x100000, stashes MB2 magic/info/entry at 0x9000,
   drops to 32-bit protected mode, and jumps to startup. */
.text
.globl kexec_tramp
.globl kexec_tramp_end

kexec_tramp:
    cli
    movq  %rdx, %r8                 /* entry */
    movq  %rcx, %r9                 /* mb2 info */
    movq  %rsi, %rcx                /* count */
    movq  %rdi, %rsi                /* src */
    movq  $0x100000, %rdi           /* dst = loaded kernel */
    cld
    rep   movsb

    movl  $0x36D76289, 0x9000       /* Multiboot2 magic */
    movl  %r9d, 0x9004              /* info */
    movl  %r8d, 0x9008              /* entry */

    leaq  tramp_gdt(%rip), %rax
    movq  %rax, tramp_gdtr+2(%rip)
    lgdt  tramp_gdtr(%rip)

    pushq $0x08                     /* 32-bit CS */
    leaq  tramp32(%rip), %rax
    pushq %rax
    lretq

.align 16
tramp_gdt:
    .quad 0
    .quad 0x00CF9A000000FFFF        /* 0x08: 32-bit code, 4GiB, DPL0 */
    .quad 0x00CF92000000FFFF        /* 0x10: 32-bit data, 4GiB, DPL0 */
tramp_gdt_end:
tramp_gdtr:
    .word tramp_gdt_end - tramp_gdt - 1
    .quad 0

.code32
.align 16
tramp32:
    movw  $0x10, %ax
    movw  %ax, %ds
    movw  %ax, %es
    movw  %ax, %ss
    movw  %ax, %fs
    movw  %ax, %gs
    movl  $0x7FFF0, %esp

    /* Leave long mode: compatibility CS already loaded. */
    movl  %cr0, %eax
    andl  $0x7FFFFFFF, %eax         /* CR0.PG = 0 */
    movl  %eax, %cr0

    movl  $0xC0000080, %ecx         /* EFER */
    rdmsr
    andl  $0xFFFFFEFF, %eax         /* LME = 0 */
    wrmsr

    movl  %cr4, %eax
    andl  $0xFFFFFF5F, %eax         /* PAE | PGE off */
    movl  %eax, %cr4

    movl  0x9000, %eax              /* Multiboot2 magic */
    movl  0x9004, %ebx              /* info */
    movl  0x9008, %edx              /* entry */
    jmp   *%edx

.code64
kexec_tramp_end:
