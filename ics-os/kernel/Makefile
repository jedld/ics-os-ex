CC=gcc
CFLAGS=-std=gnu89 -fno-stack-protector -fgnu89-inline -m64 -mcmodel=large -w -nostdlib \
	-fno-builtin -ffreestanding -fno-pie -fno-pic \
	-fno-asynchronous-unwind-tables \
	-mno-red-zone -msse -msse2 \
	-fno-strict-aliasing -fcommon -c -g -I.
ASM=gcc
ASMFLAGS=$(CFLAGS)
LDFLAGS=-m elf_x86_64 -T lscript64.ld --build-id=none -z max-page-size=0x1000 -z noexecstack -nostdlib

bzImage: all
	objcopy --only-keep-debug Kernel64.bin Kernel64.sym
	objcopy --strip-debug Kernel64.bin
	# Prefer uncompressed ELF64 for Multiboot2 (gzip optional)
	cp Kernel64.bin vmdex
	cp vmdex ..
	cp Kernel64.bin Kernel32.bin || true

all: obj Kernel64.bin

obj: scheduler.o fat.o iso9660.o ramdisk.o devfs.o iomgr.o blkcache.o devmgr_error.o kernel32.o \
		startup.o asmlib.o irqwrap.o context.o ap_trampoline.o lapic.o smp.o kexec.o virtio_blk.o
		
kernel32.o: kernel32.c build.h hardware/usb/uhci.c hardware/usb/usb.h \
		hardware/chips/serial.c hardware/ATA/ide.c \
		hardware/floppy/floppy.c hardware/floppy/floppy.h \
		memory/dexmem.c memory/dexmem.h memory/memlayout.h \
		stdlib/time.c stdlib/time.h \
		hardware/keyboard/keyboard.c console/dex_DDL.c \
		console/foreground.c process/dex_taskmgr.c process/pdispatch.c \
		console/console.c console/selfhost.c console/tty.c console/tty.h\
	hardware/chips/ports.c hardware/vga/dexvga.c stdlib/qsort.c \
		hardware/dexapm.c hardware/chips/irqhandlers.c memory/dlmalloc.c \
		memory/bsdmallo.c vfs/vfs_core.c module/module.c module/elf_module.c console/dexio.c \
		stdlib/stdlib.c hardware/keyboard/mouse.c hardware/exceptions.c \
		hardware/hardware.c hardware/chips/speaker.c devmgr/dex32_devmgr.c \
		devmgr/extension.c process/environment.c devmgr/bridges.c \
		process/sync.c console/script.c process/process.c dexapi/dex32API.c \
		hardware/ATA/ide.c vfs/vfs_aux.c memory/kheap.c memory/dexmalloc.c \
		vmm/vmm.c kexec.c kexec.h
	$(CC) $(CFLAGS) -o kernel32.o kernel32.c 
	
scheduler.o: process/scheduler.c
	$(CC) $(CFLAGS) -o scheduler.o process/scheduler.c

fat.o: filesystem/fat12.c filesystem/fat12.h
	$(CC) $(CFLAGS) -o fat.o filesystem/fat12.c
	
iso9660.o:
	$(CC) $(CFLAGS) -o iso9660.o filesystem/iso9660.c

ramdisk.o: filesystem/ramdisk.c filesystem/ramdisk.h
	$(CC) $(CFLAGS) -o ramdisk.o filesystem/ramdisk.c
	
devfs.o:
	$(CC) $(CFLAGS) -o devfs.o filesystem/devfs.c
	
iomgr.o: iomgr/iosched.c iomgr/iosched.h iomgr/bio.h stdlib/time.h iomgr/blkcache.h
	$(CC) $(CFLAGS) -o iomgr.o iomgr/iosched.c

blkcache.o: iomgr/blkcache.c iomgr/blkcache.h
	$(CC) $(CFLAGS) -o blkcache.o iomgr/blkcache.c
	
devmgr_error.o:
	$(CC) $(CFLAGS) -o devmgr_error.o devmgr/devmgr_error.c

startup.o: startup/startup.S
	$(CC) $(CFLAGS) -o startup.o startup/startup.S
	
asmlib.o: startup/asmlib.S
	$(CC) $(CFLAGS) -o asmlib.o startup/asmlib.S

irqwrap.o: irqwrap.S
	$(CC) $(CFLAGS) -o irqwrap.o irqwrap.S

context.o: cpu/context.S
	$(CC) $(CFLAGS) -o context.o cpu/context.S

ap_trampoline.o: cpu/ap_trampoline.S
	$(CC) $(CFLAGS) -o ap_trampoline.o cpu/ap_trampoline.S

kexec.o: kexec.S
	$(CC) $(CFLAGS) -o kexec.o kexec.S

virtio_blk.o: hardware/virtio/virtio_blk.c hardware/virtio/virtio_blk.h hardware/virtio/virtio.h
	$(CC) $(CFLAGS) -o virtio_blk.o hardware/virtio/virtio_blk.c

lapic.o: cpu/lapic.c cpu/lapic.h
	$(CC) $(CFLAGS) -o lapic.o cpu/lapic.c

smp.o: cpu/smp.c cpu/smp.h
	$(CC) $(CFLAGS) -o smp.o cpu/smp.c
	
Kernel64.bin: startup.o asmlib.o context.o ap_trampoline.o lapic.o smp.o kernel32.o \
		scheduler.o iomgr.o blkcache.o fat.o iso9660.o ramdisk.o \
		devfs.o irqwrap.o devmgr_error.o kexec.o virtio_blk.o
	ld $(LDFLAGS) -Map mapfile.txt

clean:
	rm -f *.o
	rm -f Kernel64.bin Kernel32.bin
	rm -f Kernel64.sym Kernel32.sym
	rm -f vmdex
