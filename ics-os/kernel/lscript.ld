OUTPUT_FORMAT("elf32-i386")
ENTRY(startup)
SECTIONS {
  . = 0x00100000;
  .multiboot : {
    *(.multiboot)
  }
  .text : {
    *(.text)
  }
  textEnd = .;
  .data : {
    *(.data)
    *(.rodata)
    *(.rodata.*)
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
    *(.rel*)
  }
}
INPUT(startup.o asmlib.o kernel32.o scheduler.o iomgr.o fat.o iso9660.o
      devfs.o irqwrap.o devmgr_error.o)
OUTPUT(Kernel32.bin)
