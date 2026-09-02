/* Real-mode AP trampoline — copied to physical 0x8000.
 * Layout at 0x8000:
 *   0x000: 16-bit entry
 *   0x100: 32-bit protected entry  
 *   0x200: 64-bit long mode entry
 * Patch area at 0x8F00: gdtr, pml4, stack, entry
 */
.data
.align 8
.globl ap_pml4
ap_pml4:      .quad 0
.globl ap_stack_top
ap_stack_top: .quad 0
.globl ap_entry
ap_entry:     .quad 0

ap_gdt_src:
    .quad 0
    .quad 0x00AF9A000000FFFF   /* 64-bit code */
    .quad 0x00CF92000000FFFF   /* data */
    .quad 0x00CF9A000000FFFF   /* 32-bit code for intermediate */

/* Flat trampoline blob: 0x300 bytes */
.globl ap_tramp_blob
.globl ap_tramp_blob_end
ap_tramp_blob:
    /* ---- 0x00: 16-bit ---- */
    .code16
    cli
    xorw %ax, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    lgdt 0x8F00
    movl %cr0, %eax
    orl  $1, %eax
    movl %eax, %cr0
    ljmp $0x18, $0x8100        /* 32-bit code selector 0x18 */

    .org ap_tramp_blob + 0x100
    /* ---- 0x100: 32-bit ---- */
    .code32
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movl %cr4, %eax
    orl  $(1<<5), %eax
    movl %eax, %cr4
    movl 0x8F10, %eax
    movl %eax, %cr3
    movl $0xC0000080, %ecx
    rdmsr
    orl  $(1<<8), %eax
    wrmsr
    movl %cr0, %eax
    orl  $((1<<31) | (1<<16)), %eax /* PG | WP */
    movl %eax, %cr0
    ljmp $0x08, $0x8200

    .org ap_tramp_blob + 0x200
    /* ---- 0x200: 64-bit ---- */
    .code64
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movq 0x8F18, %rsp
    movq 0x8F20, %rax
    callq *%rax
1:  hlt
    jmp 1b

ap_tramp_blob_end:

.text
.code64
.globl smp_install_trampoline
smp_install_trampoline:
    /* Copy blob to 0x8000 */
    movq $ap_tramp_blob, %rsi
    movq $0x8000, %rdi
    movq $ap_tramp_blob_end, %rcx
    subq %rsi, %rcx
    cld
    rep movsb

    /* GDT at 0x8E00 */
    movq $ap_gdt_src, %rsi
    movq $0x8E00, %rdi
    movq $32, %rcx
    rep movsb

    /* GDTR at 0x8F00 */
    movw $31, 0x8F00
    movl $0x8E00, 0x8F02
    movl $0, 0x8F06

    movq ap_pml4, %rax
    movq %rax, 0x8F10
    movq ap_stack_top, %rax
    movq %rax, 0x8F18
    movq ap_entry, %rax
    movq %rax, 0x8F20
    ret
