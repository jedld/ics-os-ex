/* Must be the first .text object for tcc-linked kernels so GRUB sees
   the Multiboot2 header in the first 32KiB of the file. */
.text
.align 8
.globl mb2_tcc_header
mb2_tcc_header:
    .long 0xE85250D6
    .long 0
    .long mb2_tcc_header_end - mb2_tcc_header
    .long -(0xE85250D6 + 0 + (mb2_tcc_header_end - mb2_tcc_header))
    .word 0
    .word 0
    .long 8
mb2_tcc_header_end:
    .align 4
    .long 0x1BADB002
    .long 0x00000003
    .long -(0x1BADB002 + 0x00000003)
