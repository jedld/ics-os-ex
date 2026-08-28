.section .data
msg:  .asciz "BINTOOLS_MINI_OK"
path: .asciz "/ramdisk/mini.out"
	.text
	.globl _start
_start:
	/* FXN_SYSOPEN(0xA7) "/ramdisk/mini.out" O_WRONLY|O_CREAT|O_TRUNC */
	mov $0xA7, %rax
	leaq path(%rip), %rbx
	mov $0x901, %rcx
	mov $0666, %rdx
	int $0x30
	cmpl $0, %eax
	jl .fail
	mov %eax, %edi
	/* FXN_SYSWRITE(0xA5) fd, msg, 16 */
	mov $0xA5, %rax
	mov %edi, %ebx
	leaq msg(%rip), %rcx
	mov $16, %rdx
	int $0x30
	/* FXN_SYSCLOSE(0xA8) fd */
	mov $0xA8, %rax
	mov %edi, %ebx
	int $0x30
	/* FXN_EXIT(3) 0 */
	mov $3, %rax
	xor %ebx, %ebx
	int $0x30
hlt
.fail:
	mov $3, %rax
	mov $1, %ebx
	int $0x30
hlt
