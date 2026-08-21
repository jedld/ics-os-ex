# ics-os Makefile
#
# written by Joseph Anthony C. Hermocilla
#

QEMU_I386=qemu-system-i386
QEMU_X64=qemu-system-x86_64
QEMU_MEM=128M
QEMU_NET=-net nic,model=rtl8139 -net none
# QEMU can load the 32-bit multiboot kernel the same way GRUB does.
QEMU_KERNEL=-kernel kernel/Kernel64.bin

vmdex:
	make -C kernel/

.PHONY: clean usb floppy livecd apps test-selfhost test-tccboot test-exec test-iobench test-smp test-boot test-integration
clean:
	rm -f vmdex
	rm -f ics-os-livecd.iso ics-os-usb.img
	rm -fr tmp/* tmp-usb
	make -C kernel/ clean

update_kernel_only: vmdex
	sudo rm -fr mnt
	sudo mkdir mnt
	sudo mount ics-os-floppy.img mnt -tmsdos -oloop
	sudo cp -r vmdex mnt
	sudo umount mnt
	sudo chmod 666 ics-os-floppy.img

prep_image: vmdex
	rm -fr tmp
	mkdir -p tmp/apps tmp/tcc1 tmp/lib1
	cp -r vmdex tmp
	scripts/gen-help.sh
	cp base/* tmp
	if [ -d apps ] && ls apps/* >/dev/null 2>&1; then cp apps/* tmp/apps/; fi
	find sdk -maxdepth 1 -type f -exec cp {} tmp/tcc1/ \;
	if [ -d lib ] && ls lib/* >/dev/null 2>&1; then cp lib/* tmp/lib1/; fi
	chmod +x scripts/stage-selfhost.sh
	scripts/stage-selfhost.sh tmp

floppy: prep_image
	cp grub.img ics-os-floppy.img                #copy an image with grub
	sudo rm -fr mnt
	sudo mkdir mnt
	sudo mount ics-os-floppy.img mnt -tmsdos -oloop
	sudo cp -r tmp/* mnt/
	sudo umount mnt
	sudo chmod 666 ics-os-floppy.img
	rm -fr tmp/

livecd: prep_image
	mkdir -p tmp/boot/grub
	cp boot/grub/grub.cfg tmp/boot/grub/.
	grub-mkrescue -o ics-os-livecd.iso tmp/
	rm -fr tmp/

# USB thumb-drive image (FAT root filesystem, BIOS + UEFI GRUB). No root required.
usb: apps prep_image
	chmod +x scripts/mkusb.sh
	ICSOS_USB_SIZE_MB=$${ICSOS_USB_SIZE_MB:-128} scripts/mkusb.sh
	rm -fr tmp/ tmp-usb

apps:
	-$(MAKE) -C contrib/hello
	-$(MAKE) -C contrib/hello install
	$(MAKE) -C contrib/tcc
	$(MAKE) -C contrib/tcc install

boot-floppy:
	$(QEMU_I386) $(QEMU_NET) -fda ics-os-floppy.img -boot a -m $(QEMU_MEM)

boot-floppy-gdb:
	$(QEMU_I386) $(QEMU_NET) -fda ics-os-floppy.img -boot a -m $(QEMU_MEM) -s -S

boot-floppy-vnc:
	$(QEMU_I386) $(QEMU_NET) -fda ics-os-floppy.img -boot a -m $(QEMU_MEM) -vnc :1

boot-debug:
	cd kernel ;	$(QEMU_I386) -s -S -kernel Kernel32.bin -fda ../ics-os-floppy.img -m $(QEMU_MEM) -append "root (hdp0p0)"

boot-livecd:
	$(QEMU_I386) -cdrom ics-os-livecd.iso -boot cd -m $(QEMU_MEM)

boot-livecd-amd64:
	$(QEMU_X64) -cdrom ics-os-livecd.iso -boot cd -m $(QEMU_MEM)

# Treat the USB image as a hard disk (how many BIOS/CSM firmwares expose a stick).
# ELF64 Multiboot2 kernel — boot via GRUB on the disk image (QEMU -kernel cannot load ELF64).
QEMU_DISK_BOOT=-drive file=ics-os-usb.img,format=raw,if=ide -boot c

boot-usb:
	$(QEMU_X64) $(QEMU_NET) -m $(QEMU_MEM) $(QEMU_DISK_BOOT)

# Long-mode kernel on a 64-bit CPU via GRUB multiboot2.
boot-usb-amd64:
	$(QEMU_X64) $(QEMU_NET) -m $(QEMU_MEM) $(QEMU_DISK_BOOT)

# Boot from a real UHCI USB mass-storage device and mount it as root.
boot-usb-storage:
	$(QEMU_X64) $(QEMU_NET) -m $(QEMU_MEM) \
		-drive if=none,id=stick,format=raw,file=ics-os-usb.img \
		-device piix3-usb-uhci,id=uhci \
		-device usb-storage,bus=uhci.0,drive=stick

boot-usb-uefi:
	$(QEMU_X64) $(QEMU_NET) -m $(QEMU_MEM) -bios /usr/share/ovmf/OVMF.fd \
		-drive file=ics-os-livecd.iso,format=raw,if=ide -boot c

# Headless emulator smoke tests (serial console, no VGA window).
test-usb:
	timeout 45 $(QEMU_X64) -display none -serial stdio -no-reboot -m $(QEMU_MEM) \
		$(QEMU_DISK_BOOT)

test-usb-amd64: vmdex
	@mkdir -p /tmp/icsos-boot-iso/boot/grub
	@cp kernel/Kernel64.bin /tmp/icsos-boot-iso/vmdex
	@printf '%s\n' 'set timeout=0' 'menuentry "ics" { multiboot2 /vmdex; boot }' \
		> /tmp/icsos-boot-iso/boot/grub/grub.cfg
	@grub-mkrescue -o /tmp/icsos-boot.iso /tmp/icsos-boot-iso >/dev/null 2>&1
	timeout 45 $(QEMU_X64) -nographic -no-reboot -m $(QEMU_MEM) \
		-cdrom /tmp/icsos-boot.iso -boot d

# Quick Multiboot2 CD boot: root filesystem mounts on cds0.
test-boot: vmdex
	@echo "Building Multiboot2 ISO for boot smoke test..."
	@mkdir -p /tmp/icsos-boot-iso/boot/grub
	@cp kernel/Kernel64.bin /tmp/icsos-boot-iso/vmdex
	@printf '%s\n' 'set timeout=0' 'menuentry "ics" { multiboot2 /vmdex; boot }' \
		> /tmp/icsos-boot-iso/boot/grub/grub.cfg
	@grub-mkrescue -o /tmp/icsos-boot.iso /tmp/icsos-boot-iso >/dev/null 2>&1
	@echo "Running boot smoke test..."
	@-timeout 40 $(QEMU_X64) -smp 2 -nographic -no-reboot -m $(QEMU_MEM) \
		-cdrom /tmp/icsos-boot.iso -boot d > /tmp/icsos-boot.log 2>&1
	@grep -aq "serial console ready" /tmp/icsos-boot.log
	@grep -aq "Root mount \[OK\]" /tmp/icsos-boot.log
	@grep -aq "AP scheduling enabled" /tmp/icsos-boot.log
	@! grep -aq "General Protection fault" /tmp/icsos-boot.log
	@echo "test-boot PASS"

test-usb-storage:
	timeout 60 $(QEMU_X64) -display none -serial stdio -no-reboot -m $(QEMU_MEM) \
		-drive if=none,id=stick,format=raw,file=ics-os-usb.img \
		-device piix3-usb-uhci,id=uhci \
		-device usb-storage,bus=uhci.0,drive=stick

test-smp: vmdex
	@echo "Building Multiboot2 ISO for SMP test..."
	@mkdir -p /tmp/icsos-smp-iso/boot/grub
	@cp kernel/Kernel64.bin /tmp/icsos-smp-iso/vmdex
	@printf '%s\n' 'set timeout=0' 'menuentry "ics" { multiboot2 /vmdex; boot }' \
		> /tmp/icsos-smp-iso/boot/grub/grub.cfg
	@grub-mkrescue -o /tmp/icsos-smp.iso /tmp/icsos-smp-iso >/dev/null 2>&1
	@echo "Running SMP dual-core smoke test..."
	@-timeout 45 $(QEMU_X64) -smp 2 -nographic -no-reboot -m $(QEMU_MEM) \
		-cdrom /tmp/icsos-smp.iso -boot d > /tmp/icsos-smp.log 2>&1
	@grep -q "SMP: .*CPUs online" /tmp/icsos-smp.log
	@grep -q "SMP: CPU 1" /tmp/icsos-smp.log
	@grep -aq "Root mount \[OK\]" /tmp/icsos-smp.log
	@grep -aq "AP scheduling enabled" /tmp/icsos-smp.log
	@grep -aqE "Running console thread|dex32_startup" /tmp/icsos-smp.log
	@grep -aq "AP work-steal OK" /tmp/icsos-smp.log
	@! grep -aq "General Protection fault" /tmp/icsos-smp.log
	@echo "test-smp PASS"

# Boot, compile hello.c with in-OS TinyCC, run it, then reboot.
# NOTE: i386 TinyCC selfhost is paused on the x86_64 kernel; test kept for later port.
test-selfhost: apps
	@echo "SKIP: i386 TinyCC selfhost paused until x86_64 TCC port"
	@false

# Boot and run the I/O / block-cache microbenchmark.
test-iobench: apps
	@echo "SKIP: iobench needs stable disk_mgr on x86_64 (follow-up)"
	@false

# Boot, rebuild TinyCC with itself inside ICS-OS (long-running).
test-tccboot: apps
	@echo "SKIP: tccboot paused on x86_64 kernel"
	@false

# Faster check: boot and exec the host-built hello.exe from the CD root.
test-exec: kernel/Kernel64.bin apps/hello.exe
	@echo "Building Multiboot2 ISO with hello.exe for exec smoke test..."
	@mkdir -p /tmp/icsos-exec-iso/boot/grub /tmp/icsos-exec-iso/apps
	@cp kernel/Kernel64.bin /tmp/icsos-exec-iso/vmdex
	@cp apps/hello.exe /tmp/icsos-exec-iso/apps/hello.exe
	@printf '%s\n' 'exectest' 'reboot' > /tmp/icsos-exec-iso/autoexec.bat
	@printf '%s\n' 'set timeout=0' 'menuentry "ics" { multiboot2 /vmdex; boot }' \
		> /tmp/icsos-exec-iso/boot/grub/grub.cfg
	@grub-mkrescue -o /tmp/icsos-exec.iso /tmp/icsos-exec-iso >/dev/null 2>&1
	@echo "Running ELF64 exec smoke test..."
	@-timeout 55 $(QEMU_X64) -smp 2 -nographic -no-reboot -m 256M \
		-cdrom /tmp/icsos-exec.iso -boot d > /tmp/icsos-exec.log 2>&1
	@grep -aq "Root mount \[OK\]" /tmp/icsos-exec.log
	@grep -aq "exectest: running" /tmp/icsos-exec.log
	@grep -aq "Hello World from ICS-OS!" /tmp/icsos-exec.log
	@grep -aq "EXEC_TEST_PASS" /tmp/icsos-exec.log
	@echo "test-exec PASS"

# Full QEMU integration suite for the x86_64 / SMP / ELF64 path.
test-integration: vmdex apps/hello.exe
	@echo "=== ICS-OS integration tests ==="
	$(MAKE) test-boot
	$(MAKE) test-smp
	$(MAKE) test-exec
	@echo "=== test-integration PASS ==="
