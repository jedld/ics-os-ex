/* Kernel assembler helpers — GNU as / TinyCC AT&T syntax. */
.data
.globl CsrX
.globl CsrY
.globl attb
.globl lock_var
.globl lock_val

CsrX:     .byte 0
CsrY:     .byte 0
attb:     .byte 0
          .byte 0
idtr:
    .word 2047
    .long 0x2000
testvalue:
    .long 0xFAFAFAFA
lock_var: .word 0
lock_val: .word 0

.extern curp
.extern kernelp
.extern pagedir1
.extern servicedirloc
.extern tlb_address
.extern pcibiosentry
.extern pcibios

.equ NULL_SEL,        0
.equ LINEAR_SEL,      0x08
.equ SYS_CODE_SEL2,   0x10
.equ SYS_STACK_SEL,   0x18
.equ SYS_DATA_SEL,    0x20
.equ TEXT_VIDEO_SEL,  0x28
.equ KERNEL_MEM_SEL,  0x30
.equ SYS_CODE_SEL,    0x38
.equ SYS_TSS,         0x80
.equ SCHED_TSS,       0x78
.equ USER_CODE,       0x88
.equ USER_DATA,       0x90
.equ USER_STACK,      0x98
.equ USER_TSS,        0xA0
.equ SYS_SCHED_SEL,   0xA8
.equ SYS_ERROR_TSS,   0xB0
.equ APM_CS32,        0xC0
.equ APM_DS,          0xC8
.equ APM_CS16,        0xD0
.equ KEYB_TSS,        0xD8
.equ MOUSE_TSS,       0xE0

.text
.globl mutex_lock
mutex_lock:
    movl $1, %eax
    xchgw %ax, lock_var
    movw %ax, lock_val
    ret

.globl mutex_unlock
mutex_unlock:
    movl $0, %eax
    xchgw %ax, lock_var
    movw %ax, lock_val
    ret

.globl pci_finddevice
pci_finddevice:
    pushl %ebp
    movl %esp, %ebp
    pushl %ecx
    pushl %edx
    pushl %esi
    pushl %ebx
    movb $0xB1, %ah
    movb $2, %al
    movl 8(%ebp), %ecx
    movl 12(%ebp), %edx
    movl 16(%ebp), %esi
    lcall *pcibios
    movl 20(%ebp), %ecx
    movb %bh, (%ecx)
    movl 24(%ebp), %ecx
    movb %bl, (%ecx)
    shrl $8, %eax
    andl $0xf, %eax
    popl %ebx
    popl %esi
    popl %edx
    popl %ecx
    popl %ebp
    ret

.globl pci_findclass
pci_findclass:
    pushl %ebp
    movl %esp, %ebp
    pushl %ecx
    pushl %edx
    pushl %esi
    pushl %ebx
    movb $0xB1, %ah
    movb $3, %al
    movl 8(%ebp), %ecx
    movl 12(%ebp), %esi
    lcall *pcibios
    movl 16(%ebp), %ecx
    movb %bh, (%ecx)
    movl 20(%ebp), %ecx
    movb %bl, (%ecx)
    shrl $8, %eax
    andl $0xf, %eax
    popl %ebx
    popl %esi
    popl %edx
    popl %ecx
    popl %ebp
    ret

.globl pci_readconfigbyte
pci_readconfigbyte:
    pushl %ebp
    movl %esp, %ebp
    pushl %ecx
    pushl %edx
    pushl %esi
    pushl %ebx
    movb $0xb1, %ah
    movb $8, %al
    movb 8(%ebp), %bh
    movb 12(%ebp), %bl
    movl 16(%ebp), %edi
    lcall *pcibios
    movl %ecx, %eax
    popl %ebx
    popl %esi
    popl %edx
    popl %ecx
    popl %ebp
    ret

.globl pci_readconfigword
pci_readconfigword:
    pushl %ebp
    movl %esp, %ebp
    pushl %ecx
    pushl %edx
    pushl %esi
    pushl %ebx
    movb $0xb1, %ah
    movb $9, %al
    movb 8(%ebp), %bh
    movb 12(%ebp), %bl
    movl 16(%ebp), %edi
    lcall *pcibios
    movl %ecx, %eax
    popl %ebx
    popl %esi
    popl %edx
    popl %ecx
    popl %ebp
    ret

.globl pci_readconfigdword
pci_readconfigdword:
    pushl %ebp
    movl %esp, %ebp
    pushl %ecx
    pushl %edx
    pushl %esi
    pushl %ebx
    movb $0xb1, %ah
    movb $10, %al
    movb 8(%ebp), %bh
    movb 12(%ebp), %bl
    movl 16(%ebp), %edi
    lcall *pcibios
    movl %ecx, %eax
    popl %ebx
    popl %esi
    popl %edx
    popl %ecx
    popl %ebp
    ret

.globl pci_writeconfigbyte
pci_writeconfigbyte:
    pushl %ebp
    movl %esp, %ebp
    pushl %ecx
    pushl %edx
    pushl %esi
    pushl %ebx
    movb $0xb1, %ah
    movb $10, %al
    movb 8(%ebp), %bh
    movb 12(%ebp), %bl
    movl 16(%ebp), %edi
    movl 20(%ebp), %ecx
    lcall *pcibios
    shrl $8, %eax
    popl %ebx
    popl %esi
    popl %edx
    popl %ecx
    popl %ebp
    ret

.globl pci_writeconfigword
pci_writeconfigword:
    pushl %ebp
    movl %esp, %ebp
    pushl %ecx
    pushl %edx
    pushl %esi
    pushl %ebx
    movb $0xb1, %ah
    movb $11, %al
    movb 8(%ebp), %bh
    movb 12(%ebp), %bl
    movl 16(%ebp), %edi
    movl 20(%ebp), %ecx
    lcall *pcibios
    shrl $8, %eax
    popl %ebx
    popl %esi
    popl %edx
    popl %ecx
    popl %ebp
    ret

.globl pci_writeconfigdword
pci_writeconfigdword:
    pushl %ebp
    movl %esp, %ebp
    pushl %ecx
    pushl %edx
    pushl %esi
    pushl %ebx
    movb $0xb1, %ah
    movb $12, %al
    movb 8(%ebp), %bh
    movb 12(%ebp), %bl
    movl 16(%ebp), %edi
    movl 20(%ebp), %ecx
    lcall *pcibios
    shrl $8, %eax
    popl %ebx
    popl %esi
    popl %edx
    popl %ecx
    popl %ebp
    ret

.globl pcibios_call
pcibios_call:
    pushl %ebp
    movl %esp, %ebp
    pushl %ebx
    pushl %ecx
    movl $0x49435024, %eax
    movb $0, %bl
    lcall *pcibiosentry
    cmpb $0x80, %al
    je pci_error
    cmpb $0x81, %al
    je pci_error
    movl %ebx, %eax
    addl %edx, %eax
    jmp pci_done
pci_error:
    movl $-1, %eax
pci_done:
    popl %ecx
    popl %ebx
    popl %ebp
    ret

.globl invtlb
invtlb:
    invlpg tlb_address
    ret

.globl repinword
repinword:
    pushl %ebp
    movl %esp, %ebp
    pushl %edi
    pushl %eax
    pushl %ecx
    pushl %edx
    movw 8(%ebp), %ax
    movw %ax, %es
    movl 12(%ebp), %edi
    movl 16(%ebp), %ecx
    movw 20(%ebp), %dx
    cld
    rep insw
    popl %edx
    popl %ecx
    popl %eax
    popl %edi
    popl %ebp
    ret

.globl repindword
repindword:
    pushl %ebp
    movl %esp, %ebp
    pushl %edi
    pushl %eax
    pushl %ecx
    pushl %edx
    movw 8(%ebp), %ax
    movw %ax, %es
    movl 12(%ebp), %edi
    movl 16(%ebp), %ecx
    movw 20(%ebp), %dx
    cld
    rep; insl
    popl %edx
    popl %ecx
    popl %eax
    popl %edi
    popl %ebp
    ret

.globl repoutdword
repoutdword:
    pushl %ebp
    movl %esp, %ebp
    pushl %esi
    pushl %eax
    pushl %ecx
    pushl %edx
    pushl %esi
    pushl %ds
    movw 8(%ebp), %ax
    movw %ax, %ds
    movl 12(%ebp), %esi
    movl 16(%ebp), %ecx
    movw 20(%ebp), %dx
    cld
    rep; outsl
    popl %ds
    popl %esi
    popl %edx
    popl %ecx
    popl %eax
    popl %esi
    popl %ebp
    ret

.globl repoutword
repoutword:
    pushl %ebp
    movl %esp, %ebp
    pushl %esi
    pushl %eax
    pushl %ecx
    pushl %edx
    pushl %ds
    movw 8(%ebp), %ax
    movw %ax, %ds
    movl 12(%ebp), %esi
    movl 16(%ebp), %ecx
    movw 20(%ebp), %dx
    cld
    rep outsw
    popl %ds
    popl %edx
    popl %ecx
    popl %eax
    popl %esi
    popl %ebp
    ret

.globl repoutbyte
repoutbyte:
    pushl %ebp
    movl %esp, %ebp
    pushl %esi
    pushl %eax
    pushl %ecx
    pushl %edx
    pushl %ds
    movw 8(%ebp), %ax
    movw %ax, %ds
    movl 12(%ebp), %esi
    movl 16(%ebp), %ecx
    movw 20(%ebp), %dx
    cld
    rep outsb
    popl %ds
    popl %edx
    popl %ecx
    popl %eax
    popl %esi
    popl %ebp
    ret

.globl reptrans
reptrans:
    pushl %ebp
    movl %esp, %ebp
    pushl %edi
    pushl %eax
    pushl %ecx
    pushl %edx
    movw 8(%ebp), %ax
    movw %ax, %es
    movl 12(%ebp), %edi
    movl 16(%ebp), %ecx
    movw 20(%ebp), %dx
    cld
    rep insb
    popl %edx
    popl %ecx
    popl %eax
    popl %edi
    popl %ebp
    ret

.globl getcpuid
getcpuid:
    pushl %ebp
    movl %esp, %ebp
    subl $16, %esp
    pushl %ebx
    movl 8(%ebp), %eax
    cpuid
    movl %eax, -4(%ebp)
    movl %ebx, -8(%ebp)
    movl %ecx, -12(%ebp)
    movl %edx, -16(%ebp)
    movl -4(%ebp), %eax
    movl 12(%ebp), %edx
    movl %eax, (%edx)
    movl -8(%ebp), %ecx
    movl 16(%ebp), %eax
    movl %ecx, (%eax)
    movl 20(%ebp), %edx
    movl -12(%ebp), %ecx
    movl %ecx, (%edx)
    movl 24(%ebp), %eax
    movl -16(%ebp), %edx
    movl %edx, (%eax)
    popl %ebx
    movl %ebp, %esp
    popl %ebp
    ret

.globl storeflags
storeflags:
    pushl %ebp
    movl %esp, %ebp
    pushl %ecx
    pushfl
    popl %eax
    movl %eax, -4(%ebp)
    movl %eax, %eax
    movl 8(%ebp), %edx
    movl %eax, (%edx)
    popl %ecx
    popl %ebp
    ret

.globl restoreflags
restoreflags:
    pushl %ebp
    movl %esp, %ebp
    movl 8(%ebp), %eax
    pushl %eax
    popfl
    popl %ebp
    ret

.globl loadtsr
loadtsr:
    movw $SCHED_TSS, %ax
    ltr %ax
    ret

.globl switchuserprocess
switchuserprocess:
    ljmp $USER_TSS, $0
    ret

.globl switchprocess
switchprocess:
    ljmp $SYS_TSS, $0
    ret

.globl loadregisters
loadregisters:
    lidt idtr
    ret

.globl offcomputer
offcomputer:
    pushl %ebp
    movl %esp, %ebp
    movl 8(%ebp), %edx
    movw $APM_CS32, %ax
    movw %ax, %es
    movb $0x53, %ah
    movb $0x07, %al
    movw $1, %bx
    movw $3, %cx
    lcall %es:(%edx)
    popl %ebp
    ret

.globl setpagedir
setpagedir:
    pushl %ebp
    movl %esp, %ebp
    pushl %eax
    movl 8(%ebp), %eax
    movl %eax, %cr3
    popl %eax
    popl %ebp
    ret

.globl refreshpages
refreshpages:
    movl %cr3, %eax
    movl %eax, %cr3
    ret

.globl disablepaging
disablepaging:
    movl %cr0, %eax
    andl $0x7FFFFFFF, %eax
    movl %eax, %cr0
    ljmp $SYS_CODE_SEL, $here3
here3:
    ret

.globl enablepaging
enablepaging:
    movl $0x80000011, %eax
    movl %eax, %cr0
    ljmp $SYS_CODE_SEL, $here2
here2:
    ret

.globl getCR0
getCR0:
    movl %cr0, %eax
    ret

.globl setCR0
setCR0:
    pushl %ebp
    movl %esp, %ebp
    movl 8(%ebp), %eax
    movl %eax, %cr0
    popl %ebp
    ret

xtoa:
    pushl %ebp
    movl %esp, %ebp
    pushl %ecx
    pushl %ebx
    pushl %esi
    pushl %edi
    movl 8(%ebp), %edi
    movl 12(%ebp), %ebx
    movl 20(%ebp), %eax
    testl %eax, %eax
    je xtoa_2
    movb $45, (%ebx)
    incl %ebx
    negl %edi
xtoa_2:
    movl %ebx, %esi
xtoa_3:
    movl %edi, %eax
    xorl %edx, %edx
    divl 16(%ebp)
    movl %edx, -4(%ebp)
    xorl %edx, %edx
    movl 16(%ebp), %ecx
    movl %edi, %eax
    divl %ecx
    movl -4(%ebp), %ecx
    movl %eax, %edi
    cmpl $9, %ecx
    jbe xtoa_4
    movb -4(%ebp), %al
    addb $87, %al
    movb %al, (%ebx)
    incl %ebx
    jmp xtoa_5
xtoa_4:
    movb -4(%ebp), %dl
    addb $48, %dl
    movb %dl, (%ebx)
    incl %ebx
xtoa_5:
    testl %edi, %edi
    ja xtoa_3
    movb $0, (%ebx)
    decl %ebx
xtoa_7:
    movb (%ebx), %al
    movb (%esi), %dl
    movb %dl, (%ebx)
    movb %al, (%esi)
    decl %ebx
    incl %esi
    cmpl %esi, %ebx
    ja xtoa_7
    popl %edi
    popl %esi
    popl %ebx
    popl %ecx
    popl %ebp
    ret

.globl itoa
itoa:
    pushl %ebp
    movl %esp, %ebp
    pushl %ebx
    pushl %ecx
    pushl %edx
    movl 16(%ebp), %edx
    movl 12(%ebp), %ebx
    movl 8(%ebp), %eax
    cmpl $10, %edx
    jne itoa_u
    testl %eax, %eax
    jge itoa_u
    pushl $1
    pushl %edx
    pushl %ebx
    pushl %eax
    call xtoa
    addl $16, %esp
    jmp itoa_done
itoa_u:
    pushl $0
    pushl %edx
    pushl %ebx
    pushl %eax
    call xtoa
    addl $16, %esp
itoa_done:
    movl %ebx, %eax
    popl %edx
    popl %ecx
    popl %ebx
    popl %ebp
    ret

textcolor:
    pushl %ebp
    movl %esp, %ebp
    andb $112, attb
    movb 8(%ebp), %al
    orb %al, attb
    popl %ebp
    ret

textbackground:
    pushl %ebp
    movl %esp, %ebp
    movl 8(%ebp), %eax
    andb $143, attb
    shlb $4, %al
    orb %al, attb
    popl %ebp
    ret

.globl gotoxy
gotoxy:
    pushl %ebp
    movl %esp, %ebp
    movb 8(%ebp), %al
    movb %al, CsrX
    movb 12(%ebp), %dl
    movb %dl, CsrY
    pushl %edx
    pushl %eax
    call move_cursor
    addl $8, %esp
    popl %ebp
    ret

MOVECURSOR:
    pushl %edx
    pushl %eax
    pushl %ebx
    movw $0x3D4, %dx
    movb $0xF, %al
    outb %al, %dx
    incw %dx
    movb %bl, %al
    outb %al, %dx
    decw %dx
    movb $0xE, %al
    outb %al, %dx
    incw %dx
    movb %bh, %al
    outb %al, %dx
    popl %ebx
    popl %eax
    popl %edx
    ret

.globl move_cursor
move_cursor:
    pushl %ebp
    movl %esp, %ebp
    pushl %ecx
    pushl %ebx
    movzwl 8(%ebp), %eax
    movl %eax, %edx
    shll $4, %edx
    leal (%edx, %edx, 4), %edx
    addw 12(%ebp), %dx
    movw %dx, -2(%ebp)
    movzwl -2(%ebp), %ebx
    call MOVECURSOR
    popl %ebx
    popl %ecx
    popl %ebp
    ret
