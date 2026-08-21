/* ICS-OS kernel entry — GNU as / TinyCC AT&T syntax. Multiboot + GDT. */
.section .multiboot, "a"
.align 4
mb_header:
    .long 0x1BADB002
    .long 0x00000003
    .long -(0x1BADB002 + 0x00000003)

.text
.globl startup
.globl reset_gdtr
.globl multiboothdr
.extern main

.equ NULL_SEL,        0
.equ LINEAR_SEL,      0x08
.equ SYS_CODE_SEL2,   0x10
.equ SYS_STACK_SEL,   0x18
.equ SYS_DATA_SEL,    0x20
.equ TEXT_VIDEO_SEL,  0x28
.equ KERNEL_MEM_SEL,  0x30
.equ SYS_CODE_SEL,    0x38

.equ gdt,  0x01000
.equ gdt1, 0x01008
.equ gdt2, 0x01010
.equ gdt3, 0x01018
.equ gdt4, 0x01020
.equ gdt5, 0x01028
.equ gdt6, 0x01030
.equ gdt7, 0x01038

startup:
    jmp multiboot

multiboot:
    movl %ebx, multiboothdr
    movb $'.', 0xb8000

    movw $0, gdt
    movw $0, gdt+2
    movl $0, gdt+4

    movw $0xFFFF, gdt1
    movw $0, gdt1+2
    movb $0, gdt1+4
    movb $0x92, gdt1+5
    movb $0xCF, gdt1+6
    movb $0, gdt1+7

    xorl %ebx, %ebx
    movl $0, %ebx
    movl %ebx, %eax

    movw $0xFFFF, gdt2
    movw %ax, gdt2+2
    shrl $16, %eax
    movb %al, gdt2+4
    movb $0x9A, gdt2+5
    movb $0xCF, gdt2+6
    movb %ah, gdt2+7

    movl $0x80000, %eax
    movw %ax, gdt3+2
    shrl $16, %eax
    movb %al, gdt3+4
    movw $0xFFFF, gdt3
    movb $0x92, gdt3+5
    movb $0xCF, gdt3+6
    movb %ah, gdt3+7

    movl $0, %eax
    movw %ax, gdt4+2
    shrl $16, %eax
    movb %al, gdt4+4
    movw $0xFFFF, gdt4
    movb $0x92, gdt4+5
    movb $0xCF, gdt4+6
    movb %ah, gdt4+7

    movb $'.', 0xb8002
    cli
    lgdt gdtr
    ljmp $SYS_CODE_SEL2, $not_multiboot

not_multiboot:
    movb $'.', 0xb8004
    movw $LINEAR_SEL, %ax
    movw %ax, %es
    movw %ax, %ss
    movw %ax, %ds
    movw %ax, %gs
    movw %ax, %fs

    movl $0, %eax
    movw $0xFFFF, %es:gdt7
    movw %ax, %es:gdt7+2
    shrl $16, %eax
    movb %al, %es:gdt7+4
    movb $0x9A, %es:gdt7+5
    movb $0xCF, %es:gdt7+6
    movb %ah, %es:gdt7+7

    movl $0, %eax
    movw %ax, %es:gdt3+2
    shrl $16, %eax
    movb %al, %es:gdt3+4
    movw $0xFFFF, %es:gdt3
    movb $0x92, %es:gdt3+5
    movb $0xCF, %es:gdt3+6
    movb %ah, %es:gdt3+7

    movl $0, %eax
    movw %ax, %es:gdt4+2
    shrl $16, %eax
    movb %al, %es:gdt4+4
    movw $0xFFFF, %es:gdt4
    movb $0x92, %es:gdt4+5
    movb $0xCF, %es:gdt4+6
    movb %ah, %es:gdt4+7

    movl $0x0B8000, %eax
    movw %ax, %es:gdt5+2
    shrl $16, %eax
    movb %al, %es:gdt5+4
    movw $0xFFFF, %es:gdt5
    movb $0x92, %es:gdt5+5
    movb $0xCF, %es:gdt5+6
    movb %ah, %es:gdt5+7

    movl $0x01000000, %eax
    movw $0x0244, %es:gdt6
    movw %ax, %es:gdt6+2
    shrl $16, %eax
    movb %al, %es:gdt6+4
    movb $0x92, %es:gdt6+5
    movb $0xCF, %es:gdt6+6
    movb %ah, %es:gdt6+7

    movb $'.', 0xb8006

    movl %cr0, %eax
    orb $1, %al
    movl %eax, %cr0
    ljmp $SYS_CODE_SEL, $linearcode

linearcode:
    xorl %eax, %eax
    movw $SYS_STACK_SEL, %ax
    movw %ax, %ss
    movl $0x9FFFE, %esp
    xorl %eax, %eax
    movw $SYS_DATA_SEL, %ax
    movw %ax, %ds
    call enable_A20
    call disable_apic
    call main

enable_A20:
    inb $0x92, %al
    testb $2, %al
    jnz .a20_done
    orb $2, %al
    andb $0xFE, %al
    outb %al, $0x92
.a20_done:
    ret

disable_apic:
    pushal
    movl $1, %eax
    cpuid
    testl $0x200, %edx
    jz .no_apic
    movl $0x1B, %ecx
    rdmsr
    andl $0xFFFFF7FF, %eax
    wrmsr
.no_apic:
    popal
    ret

reset_gdtr:
    lgdt gdtr
    ret

.data
multiboothdr:
    .long 0
gdtr:
    .word 2047
    .long 0x01000
