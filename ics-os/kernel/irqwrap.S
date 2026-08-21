/* Interrupt wrappers — GNU as / TinyCC AT&T syntax (32-bit). */
.text

.extern time_handler
.extern kbd_irq
.extern mouse_irq
.extern pagefaulthandler
.extern fdchandler
.extern api_syscall
.extern GPFhandler
.extern CPUint
.extern nocoprocessor
.extern divide_error
.extern exc_invalidtss
.extern irq_activate

.globl syscallwrapper
syscallwrapper:
    pushl %edi
    pushl %esi
    pushl %edx
    pushl %ecx
    pushl %ebx
    pushl %eax
    call api_syscall
    addl $24, %esp
    iret

.globl CPUintwrapper
CPUintwrapper:
    cli
    call CPUint
    sti
    iret

.globl fdcwrapper
fdcwrapper:
    cli
    pushal
    call fdchandler
    popal
    sti
    iret

.globl copwrapper
copwrapper:
    cli
    call nocoprocessor
    sti
    iret

.globl timerwrapper
timerwrapper:
    cli
    pushl %ebp
    pushl %gs
    pushl %fs
    pushl %es
    pushl %ss
    pushl %ds
    pushal
    call time_handler
    popal
    popl %ds
    popl %ss
    popl %es
    popl %fs
    popl %gs
    popl %ebp
    sti
    iret

.globl kbdwrapper
kbdwrapper:
    pushl %ebp
    pushl %gs
    pushl %fs
    pushl %es
    pushl %ss
    pushl %ds
    pushal
    call kbd_irq
    popal
    popl %ds
    popl %ss
    popl %es
    popl %fs
    popl %gs
    popl %ebp
    iret

.globl mousewrapper
mousewrapper:
    pushl %ebp
    pushl %gs
    pushl %fs
    pushl %es
    pushl %ss
    pushl %ds
    pushal
    call mouse_irq
    movb $0x20, %al
    outb %al, $0xA0
    outb %al, $0x20
    popal
    popl %ds
    popl %ss
    popl %es
    popl %fs
    popl %gs
    popl %ebp
    iret

.globl irq1wrapper
irq1wrapper:
    pushl %ebp
    pushl %gs
    pushl %fs
    pushl %es
    pushl %ss
    pushl %ds
    pushal
    pushl $1
    call irq_activate
    addl $4, %esp
    popal
    popl %ds
    popl %ss
    popl %es
    popl %fs
    popl %gs
    popl %ebp
    iret

.globl irq2wrapper
irq2wrapper:
    pushl %ebp
    pushl %gs
    pushl %fs
    pushl %es
    pushl %ss
    pushl %ds
    pushal
    pushl $2
    call irq_activate
    addl $4, %esp
    popal
    popl %ds
    popl %ss
    popl %es
    popl %fs
    popl %gs
    popl %ebp
    iret

.globl irq3wrapper
irq3wrapper:
    pushl %ebp
    pushl %gs
    pushl %fs
    pushl %es
    pushl %ss
    pushl %ds
    pushal
    pushl $3
    call irq_activate
    addl $4, %esp
    popal
    popl %ds
    popl %ss
    popl %es
    popl %fs
    popl %gs
    popl %ebp
    iret

.globl irq4wrapper
irq4wrapper:
    pushl %ebp
    pushl %gs
    pushl %fs
    pushl %es
    pushl %ss
    pushl %ds
    pushal
    pushl $4
    call irq_activate
    addl $4, %esp
    popal
    popl %ds
    popl %ss
    popl %es
    popl %fs
    popl %gs
    popl %ebp
    iret

.globl irq5wrapper
irq5wrapper:
    pushl %ebp
    pushl %gs
    pushl %fs
    pushl %es
    pushl %ss
    pushl %ds
    pushal
    pushl $5
    call irq_activate
    addl $4, %esp
    popal
    popl %ds
    popl %ss
    popl %es
    popl %fs
    popl %gs
    popl %ebp
    iret

.globl irq6wrapper
irq6wrapper:
    pushl %ebp
    pushl %gs
    pushl %fs
    pushl %es
    pushl %ss
    pushl %ds
    pushal
    pushl $6
    call irq_activate
    addl $4, %esp
    popal
    popl %ds
    popl %ss
    popl %es
    popl %fs
    popl %gs
    popl %ebp
    iret

.globl irq7wrapper
irq7wrapper:
    pushl %ebp
    pushl %gs
    pushl %fs
    pushl %es
    pushl %ss
    pushl %ds
    pushal
    pushl $7
    call irq_activate
    addl $4, %esp
    popal
    popl %ds
    popl %ss
    popl %es
    popl %fs
    popl %gs
    popl %ebp
    iret

.globl irq8wrapper
irq8wrapper:
    pushl %ebp
    pushl %gs
    pushl %fs
    pushl %es
    pushl %ss
    pushl %ds
    pushal
    pushl $8
    call irq_activate
    addl $4, %esp
    popal
    popl %ds
    popl %ss
    popl %es
    popl %fs
    popl %gs
    popl %ebp
    iret

.globl irq9wrapper
irq9wrapper:
    pushl %ebp
    pushl %gs
    pushl %fs
    pushl %es
    pushl %ss
    pushl %ds
    pushal
    pushl $9
    call irq_activate
    addl $4, %esp
    popal
    popl %ds
    popl %ss
    popl %es
    popl %fs
    popl %gs
    popl %ebp
    iret

.globl irq10wrapper
irq10wrapper:
    pushl %ebp
    pushl %gs
    pushl %fs
    pushl %es
    pushl %ss
    pushl %ds
    pushal
    pushl $10
    call irq_activate
    addl $4, %esp
    popal
    popl %ds
    popl %ss
    popl %es
    popl %fs
    popl %gs
    popl %ebp
    iret

.globl irq11wrapper
irq11wrapper:
    pushl %ebp
    pushl %gs
    pushl %fs
    pushl %es
    pushl %ss
    pushl %ds
    pushal
    pushl $11
    call irq_activate
    addl $4, %esp
    popal
    popl %ds
    popl %ss
    popl %es
    popl %fs
    popl %gs
    popl %ebp
    iret

.globl irq12wrapper
irq12wrapper:
    pushl %ebp
    pushl %gs
    pushl %fs
    pushl %es
    pushl %ss
    pushl %ds
    pushal
    pushl $12
    call irq_activate
    addl $4, %esp
    popal
    popl %ds
    popl %ss
    popl %es
    popl %fs
    popl %gs
    popl %ebp
    iret

.globl div_wrapper
div_wrapper:
    cli
    pushl %ebp
    movl %esp, %ebp
    movl 8(%ebp), %eax
    pushl %eax
    call divide_error
    addl $4, %esp
    popl %ebp
    sti
    iret

.globl gpfwrapper
gpfwrapper:
    cli
    pushl %ebp
    movl %esp, %ebp
    movl 8(%ebp), %eax
    pushl %eax
    call GPFhandler
    addl $4, %esp
    popl %ebp
    sti
    iret

.globl invalidtsswrapper
invalidtsswrapper:
    cli
    pushl %ebp
    movl %esp, %ebp
    movl 8(%ebp), %eax
    pushl %eax
    call exc_invalidtss
    addl $4, %esp
    popl %ebp
    sti
    iret

.globl pfwrapper
pfwrapper:
    cli
    pushl %ebp
    movl %esp, %ebp
    movl 8(%ebp), %eax
    pushl %eax
    movl %cr2, %eax
    pushl %eax
    call pagefaulthandler
    addl $8, %esp
    sti
    iret
