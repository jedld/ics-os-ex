OUTPUT_FORMAT("elf64-x86-64")
ENTRY(startup)
SECTIONS {
  . = 0x00100000;
  .multiboot : {
    *(.multiboot)
  }
  .text : {
    *(.text)
    *(.text.*)
  }
  textEnd = .;
  .rodata : {
    *(.rodata)
    *(.rodata.*)
  }
  .data : {
    *(.data)
    *(.data.*)
  }
  dataEnd = .;
  .bss : {
    *(.common)
    *(.bss)
    *(.bss.*)
  }
  bssEnd = .;
  /DISCARD/ : {
    *(.comment)
    *(.note*)
    *(.eh_frame*)
    *(.gnu*)
  }
}
ASSERT(bssEnd <= 0x003C0000, "kernel BSS collided with 4MiB user ELF window");
INPUT(startup.o asmlib.o context.o ap_trampoline.o lapic.o smp.o
      kernel32.o scheduler.o iomgr.o blkcache.o posixfd.o fat.o ext4.o iso9660.o ramdisk.o
      devfs.o irqwrap.o devmgr_error.o kexec.o virtio_blk.o)
OUTPUT(Kernel64.bin)
