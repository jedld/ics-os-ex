CC=gcc
CFLAGS=-std=gnu89 -fno-stack-protector -fgnu89-inline -m32 -w -nostdlib \
	-fno-builtin -ffreestanding -fno-pie -fno-pic \
	-fno-asynchronous-unwind-tables -mno-mmx -mno-sse -mno-sse2 \
	-msoft-float -fno-strict-aliasing -fcommon -c -g
ASM=nasm
ASMFLAGS=-f elf32
LDFLAGS=-m elf_i386 -T lscript.ld --build-id=none -z max-page-size=0x1000 -z noexecstack -nostdlib

bzImage: all
	objcopy --only-keep-debug Kernel32.bin Kernel32.sym
	objcopy --strip-debug Kernel32.bin
	gzip -c -9 Kernel32.bin >  vmdex
	cp vmdex ..

all: obj Kernel32.bin

obj: scheduler.o fat.o iso9660.o devfs.o iomgr.o devmgr_error.o kernel32.o \
		startup.o asmlib.o irqwrap.o 
	#strip --strip-debug *.o
		
kernel32.o: kernel32.c build.h hardware/usb/uhci.c hardware/usb/usb.h \
		hardware/chips/serial.c hardware/ATA/ide.c \
		hardware/floppy/floppy.c hardware/floppy/floppy.h \
		memory/dexmem.c memory/dexmem.h \
		stdlib/time.c stdlib/time.h \
		hardware/keyboard/keyboard.c console/dex_DDL.c \
		console/foreground.c process/dex_taskmgr.c process/pdispatch.c \
		console/console.c
	$(CC) $(CFLAGS) -o kernel32.o kernel32.c 
	
scheduler.o:
	$(CC) $(CFLAGS) -o scheduler.o process/scheduler.c

fat.o: filesystem/fat12.c filesystem/fat12.h
	$(CC) $(CFLAGS) -o fat.o filesystem/fat12.c
	
iso9660.o:
	$(CC) $(CFLAGS) -o iso9660.o filesystem/iso9660.c
	
devfs.o:
	$(CC) $(CFLAGS) -o devfs.o filesystem/devfs.c
	
iomgr.o: iomgr/iosched.c iomgr/iosched.h stdlib/time.h
	$(CC) $(CFLAGS) -o iomgr.o iomgr/iosched.c
	
devmgr_error.o:
	$(CC) $(CFLAGS) -o devmgr_error.o devmgr/devmgr_error.c

startup.o:
	$(ASM) $(ASMFLAGS) -o startup.o startup/startup.asm
	
asmlib.o:
	$(ASM) $(ASMFLAGS) -o asmlib.o startup/asmlib.asm 

irqwrap.o:
	$(ASM) $(ASMFLAGS) -o irqwrap.o irqwrap.asm
	
Kernel32.bin: startup.o asmlib.o kernel32.o scheduler.o iomgr.o fat.o iso9660.o \
		devfs.o irqwrap.o devmgr_error.o
	ld $(LDFLAGS) -Map mapfile.txt

clean:
	rm -f *.o
	rm -f Kernel32.bin
	rm -f Kernel32.sym
	rm -f vmdex
