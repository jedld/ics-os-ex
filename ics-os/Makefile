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

.PHONY: clean usb floppy livecd apps test-selfhost test-tccboot test-kbuild test-fullhost test-exec test-iobench test-virtio test-smp test-boot test-integration test-posixio
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
	-$(MAKE) -C contrib/posixio
	-$(MAKE) -C contrib/posixio install
	-$(MAKE) -C contrib/sh
	-$(MAKE) -C contrib/sh install
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

# Boot, compile min.c/hello.c with in-OS TinyCC (ELF64), run them, reboot.
test-selfhost: apps vmdex
	@echo "Building Multiboot2 ISO for x86_64 TinyCC selfhost..."
	@mkdir -p /tmp/icsos-selfhost-iso/boot/grub /tmp/icsos-selfhost-iso/apps
	@cp kernel/Kernel64.bin /tmp/icsos-selfhost-iso/vmdex
	@cp apps/tcc.exe /tmp/icsos-selfhost-iso/apps/tcc.exe
	@cp contrib/selfhost/min.c contrib/selfhost/hello.c \
		contrib/selfhost/tinyio.c contrib/selfhost/tinycrt.c \
		/tmp/icsos-selfhost-iso/apps/
	@printf '%s\n' 'set PATH=/icsos/apps' 'selfhost' 'reboot' \
		> /tmp/icsos-selfhost-iso/autoexec.bat
	@printf '%s\n' 'set timeout=0' 'menuentry "ics" { multiboot2 /vmdex; boot }' \
		> /tmp/icsos-selfhost-iso/boot/grub/grub.cfg
	@grub-mkrescue -o /tmp/icsos-selfhost.iso /tmp/icsos-selfhost-iso >/dev/null 2>&1
	@echo "Running self-host test (TinyCC inside ICS-OS)..."
	@-timeout 300 $(QEMU_X64) -enable-kvm -cpu host -smp 1 -nographic -no-reboot -m 1024M \
		-cdrom /tmp/icsos-selfhost.iso -boot d > /tmp/icsos-selfhost.log 2>&1
	@grep -aq "Root mount \[OK\]" /tmp/icsos-selfhost.log
	@grep -aq "ramdisk: mounted" /tmp/icsos-selfhost.log
	@grep -aq "SELFHOST_TEST_PASS" /tmp/icsos-selfhost.log
	@! grep -aq "SELFHOST_TEST_FAIL" /tmp/icsos-selfhost.log
	@echo "test-selfhost PASS"

# Boot and run the I/O / block-cache microbenchmark (CD sequential map).
test-iobench: kernel/Kernel64.bin apps/tcc.exe
	@echo "Building Multiboot2 ISO for iobench..."
	@rm -rf /tmp/icsos-iobench-iso
	@mkdir -p /tmp/icsos-iobench-iso/boot/grub /tmp/icsos-iobench-iso/apps
	@cp kernel/Kernel64.bin /tmp/icsos-iobench-iso/vmdex
	@cp apps/tcc.exe /tmp/icsos-iobench-iso/apps/tcc.exe
	@printf '%s\n' 'iobench' 'reboot' > /tmp/icsos-iobench-iso/autoexec.bat
	@printf '%s\n' 'set timeout=0' 'menuentry "ics" { multiboot2 /vmdex; boot }' \
		> /tmp/icsos-iobench-iso/boot/grub/grub.cfg
	@grub-mkrescue -o /tmp/icsos-iobench.iso /tmp/icsos-iobench-iso >/dev/null 2>&1
	@echo "Running iobench (KVM)..."
	@-timeout 90 stdbuf -oL $(QEMU_X64) -enable-kvm -cpu host -smp 1 -nographic -no-reboot -m 256M \
		-cdrom /tmp/icsos-iobench.iso -boot d > /tmp/icsos-iobench.log 2>&1
	@grep -aq "Root mount \[OK\]" /tmp/icsos-iobench.log
	@grep -aq "iobench: size=" /tmp/icsos-iobench.log
	@grep -aq "IOBENCH_PASS" /tmp/icsos-iobench.log
	@grep -aq "IOBENCH_CACHE_OK" /tmp/icsos-iobench.log
	@! grep -aq "iobench: FAIL" /tmp/icsos-iobench.log
	@echo "test-iobench PASS"

# virtio-blk DMA round-trip (modern PCI + MSI-X). ATA-only boots stay on
# "virtio-blk: none"; this target attaches a QEMU virtio disk.
test-virtio: kernel/Kernel64.bin
	@echo "Building Multiboot2 ISO for virtio-blk..."
	@mkdir -p /tmp/icsos-virtio-iso/boot/grub
	@cp kernel/Kernel64.bin /tmp/icsos-virtio-iso/vmdex
	@printf '%s\n' 'set timeout=0' 'menuentry "ics" { multiboot2 /vmdex; boot }' \
		> /tmp/icsos-virtio-iso/boot/grub/grub.cfg
	@grub-mkrescue -o /tmp/icsos-virtio.iso /tmp/icsos-virtio-iso >/dev/null 2>&1
	@dd if=/dev/zero of=/tmp/icsos-vblk.img bs=1M count=8 status=none
	@echo "Running virtio-blk test (KVM)..."
	@-timeout 45 stdbuf -oL $(QEMU_X64) -enable-kvm -cpu host -smp 1 -nographic -no-reboot -m 256M \
		-cdrom /tmp/icsos-virtio.iso -boot d \
		-drive file=/tmp/icsos-vblk.img,if=none,id=vd0,format=raw \
		-device virtio-blk-pci,drive=vd0,disable-legacy=on \
		> /tmp/icsos-virtio.log 2>&1
	@grep -aq "Root mount \[OK\]" /tmp/icsos-virtio.log
	@grep -aq "VIRTIO_BLK_OK" /tmp/icsos-virtio.log
	@grep -aq "VIRTIO_IRQ_OK" /tmp/icsos-virtio.log
	@grep -aq "pipelined reads \[OK\]" /tmp/icsos-virtio.log
	@! grep -aq "VIRTIO_BLK_FAIL" /tmp/icsos-virtio.log
	@! grep -aq "General Protection fault" /tmp/icsos-virtio.log
	@echo "test-virtio PASS"

# Boot, rebuild TinyCC with itself inside ICS-OS (Multiboot2 ISO).
# Packs an 8.3-renamed TinyCC tree as one ustar so staging is a single CD read.
test-tccboot: apps vmdex
	@echo "Building 8.3 TinyCC source tree + tccsrc.tar..."
	@chmod +x scripts/stage-tcc-short.sh
	@scripts/stage-tcc-short.sh /tmp/icsos-tcc-short
	@tar --format=ustar -C /tmp/icsos-tcc-short -cf /tmp/icsos-tccsrc.tar .
	@echo "Building Multiboot2 ISO for x86_64 TinyCC self-build (tccboot)..."
	@rm -rf /tmp/icsos-tccboot-iso
	@mkdir -p /tmp/icsos-tccboot-iso/boot/grub \
		/tmp/icsos-tccboot-iso/apps \
		/tmp/icsos-tccboot-iso/tcc1 \
		/tmp/icsos-tccboot-iso/include \
		/tmp/icsos-tccboot-iso/pre
	@cp kernel/Kernel64.bin /tmp/icsos-tccboot-iso/vmdex
	@cp apps/tcc.exe /tmp/icsos-tccboot-iso/apps/tcc.exe
	@cp contrib/selfhost/min.c /tmp/icsos-tccboot-iso/apps/min.c
	@cp /tmp/icsos-tccsrc.tar /tmp/icsos-tccboot-iso/tccsrc.tar
	@cp /tmp/icsos-tcc-short/pre/*.c /tmp/icsos-tccboot-iso/pre/
	@cp sdk/tccsdk.c sdk/posix.c sdk/libtcc1.c sdk/crt1.c sdk/setjmp.c \
		/tmp/icsos-tccboot-iso/tcc1/
	@cp -a sdk/include/. /tmp/icsos-tccboot-iso/include/
	@printf '%s\n' 'set PATH=/icsos/apps' 'set SDK_HOME=/icsos/tcc1' \
		'tccboot' 'reboot' > /tmp/icsos-tccboot-iso/autoexec.bat
	@printf '%s\n' 'set timeout=0' 'menuentry "ics" { multiboot2 /vmdex; boot }' \
		> /tmp/icsos-tccboot-iso/boot/grub/grub.cfg
	@grub-mkrescue -o /tmp/icsos-tccboot.iso /tmp/icsos-tccboot-iso >/dev/null 2>&1
	@echo "Running tccboot test (KVM)..."
	@-timeout 7200 $(QEMU_X64) -enable-kvm -cpu host -smp 1 -nographic -no-reboot -m 1024M \
		-cdrom /tmp/icsos-tccboot.iso -boot d > /tmp/icsos-tccboot.log 2>&1
	@grep -aq "Root mount \[OK\]" /tmp/icsos-tccboot.log
	@grep -aq "ramdisk: mounted" /tmp/icsos-tccboot.log
	@grep -aq "TCCBOOT_TEST_PASS" /tmp/icsos-tccboot.log
	@! grep -aq "TCCBOOT_TEST_FAIL" /tmp/icsos-tccboot.log
	@echo "test-tccboot PASS"

# Compile the kernel inside ICS-OS with tcc (C via tcc, GAS via prebuilt .o).
test-kbuild: apps vmdex
	@echo "Staging kernel sources + GAS objects for kbuild..."
	@chmod +x scripts/stage-kernel-src.sh
	@scripts/stage-kernel-src.sh /tmp/icsos-kbuild
	@echo "Building Multiboot2 ISO for kbuild..."
	@rm -rf /tmp/icsos-kbuild-iso
	@mkdir -p /tmp/icsos-kbuild-iso/boot/grub /tmp/icsos-kbuild-iso/apps \
		/tmp/icsos-kbuild-iso/kasm
	@cp kernel/Kernel64.bin /tmp/icsos-kbuild-iso/vmdex
	@cp apps/tcc.exe /tmp/icsos-kbuild-iso/apps/tcc.exe
	@cp /tmp/icsos-kbuild/ksrc.tar /tmp/icsos-kbuild-iso/ksrc.tar
	@cp /tmp/icsos-kbuild/kasm/*.o /tmp/icsos-kbuild-iso/kasm/
	@printf '%s\n' 'set PATH=/icsos/apps' 'kbuild' 'reboot' \
		> /tmp/icsos-kbuild-iso/autoexec.bat
	@printf '%s\n' 'set timeout=0' 'menuentry "ics" { multiboot2 /vmdex; boot }' \
		> /tmp/icsos-kbuild-iso/boot/grub/grub.cfg
	@grub-mkrescue -o /tmp/icsos-kbuild.iso /tmp/icsos-kbuild-iso >/dev/null 2>&1
	@echo "Running kbuild test (KVM)..."
	@-timeout 3600 stdbuf -oL $(QEMU_X64) -enable-kvm -cpu host -smp 1 -nographic -no-reboot -m 1024M \
		-cdrom /tmp/icsos-kbuild.iso -boot d > /tmp/icsos-kbuild.log 2>&1
	@grep -aq "Root mount \[OK\]" /tmp/icsos-kbuild.log
	@grep -aq "KBUILD_TEST_PASS" /tmp/icsos-kbuild.log
	@grep -aq "KBUILD_KEXEC" /tmp/icsos-kbuild.log
	@grep -aq "Build: kbuild-inos" /tmp/icsos-kbuild.log
	@grep -aq "KEXEC_BOOT_OK" /tmp/icsos-kbuild.log
	@! grep -aq "KBUILD_TEST_FAIL" /tmp/icsos-kbuild.log
	@echo "test-kbuild PASS"

# Compile tcc with tcc, then compile the kernel with tccnew.
test-fullhost: apps vmdex
	@echo "Staging tcc + kernel sources for fullhost..."
	@chmod +x scripts/stage-tcc-short.sh scripts/stage-kernel-src.sh
	@scripts/stage-tcc-short.sh /tmp/icsos-tcc-short
	@tar --format=ustar -C /tmp/icsos-tcc-short -cf /tmp/icsos-tccsrc.tar .
	@scripts/stage-kernel-src.sh /tmp/icsos-kbuild
	@rm -rf /tmp/icsos-fullhost-iso
	@mkdir -p /tmp/icsos-fullhost-iso/boot/grub /tmp/icsos-fullhost-iso/apps \
		/tmp/icsos-fullhost-iso/tcc1 /tmp/icsos-fullhost-iso/include \
		/tmp/icsos-fullhost-iso/kasm
	@cp kernel/Kernel64.bin /tmp/icsos-fullhost-iso/vmdex
	@cp apps/tcc.exe /tmp/icsos-fullhost-iso/apps/tcc.exe
	@cp contrib/selfhost/min.c /tmp/icsos-fullhost-iso/apps/min.c
	@cp /tmp/icsos-tccsrc.tar /tmp/icsos-fullhost-iso/tccsrc.tar
	@cp /tmp/icsos-kbuild/ksrc.tar /tmp/icsos-fullhost-iso/ksrc.tar
	@cp /tmp/icsos-kbuild/kasm/*.o /tmp/icsos-fullhost-iso/kasm/
	@cp sdk/tccsdk.c sdk/posix.c sdk/libtcc1.c sdk/crt1.c sdk/setjmp.c \
		/tmp/icsos-fullhost-iso/tcc1/
	@cp -a sdk/include/. /tmp/icsos-fullhost-iso/include/
	@printf '%s\n' 'set PATH=/icsos/apps' 'fullhost' 'reboot' \
		> /tmp/icsos-fullhost-iso/autoexec.bat
	@printf '%s\n' 'set timeout=0' 'menuentry "ics" { multiboot2 /vmdex; boot }' \
		> /tmp/icsos-fullhost-iso/boot/grub/grub.cfg
	@grub-mkrescue -o /tmp/icsos-fullhost.iso /tmp/icsos-fullhost-iso >/dev/null 2>&1
	@echo "Running fullhost test (KVM, long)..."
	@-timeout 3600 $(QEMU_X64) -enable-kvm -cpu host -smp 1 -nographic -no-reboot -m 1024M \
		-cdrom /tmp/icsos-fullhost.iso -boot d > /tmp/icsos-fullhost.log 2>&1
	@grep -aq "TCCBOOT_TEST_PASS" /tmp/icsos-fullhost.log
	@grep -aq "KBUILD_TEST_PASS" /tmp/icsos-fullhost.log
	@grep -aq "KBUILD_KEXEC" /tmp/icsos-fullhost.log
	@grep -aq "Build: fullhost-inos" /tmp/icsos-fullhost.log
	@grep -aq "KEXEC_BOOT_OK" /tmp/icsos-fullhost.log
	@grep -aq "FULLHOST_TEST_PASS" /tmp/icsos-fullhost.log
	@echo "test-fullhost PASS"
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

# POSIX fds, preadv/pwritev/fsync, io_uring, and async /dev/vblk.
test-posixio: kernel/Kernel64.bin apps/posixio.exe
	@echo "Building Multiboot2 ISO for POSIX I/O..."
	@mkdir -p /tmp/icsos-posixio-iso/boot/grub /tmp/icsos-posixio-iso/apps
	@cp kernel/Kernel64.bin /tmp/icsos-posixio-iso/vmdex
	@cp apps/posixio.exe /tmp/icsos-posixio-iso/apps/posixio.exe
	@printf '%s\n' 'posixio' 'reboot' > /tmp/icsos-posixio-iso/autoexec.bat
	@printf '%s\n' 'set timeout=0' 'menuentry "ics" { multiboot2 /vmdex; boot }' \
		> /tmp/icsos-posixio-iso/boot/grub/grub.cfg
	@grub-mkrescue -o /tmp/icsos-posixio.iso /tmp/icsos-posixio-iso >/dev/null 2>&1
	@echo "Running POSIX I/O test (KVM, virtio-blk)..."
	@dd if=/dev/zero of=/tmp/icsos-posixio-vblk.img bs=1M count=8 status=none
	@-timeout 55 stdbuf -oL $(QEMU_X64) -enable-kvm -cpu host -smp 1 -nographic -no-reboot -m 256M \
		-cdrom /tmp/icsos-posixio.iso -boot d \
		-drive file=/tmp/icsos-posixio-vblk.img,if=none,id=vd0,format=raw \
		-device virtio-blk-pci,drive=vd0,disable-legacy=on \
		> /tmp/icsos-posixio.log 2>&1
	@grep -aq "Root mount \[OK\]" /tmp/icsos-posixio.log
	@grep -aq "ramdisk: mounted" /tmp/icsos-posixio.log
	@grep -aq "POSIXIO_PASS" /tmp/icsos-posixio.log
	@grep -aq "URING_PASS" /tmp/icsos-posixio.log
	@grep -aq "URING_VBLK_PASS" /tmp/icsos-posixio.log
	@! grep -aq "POSIXIO_FAIL" /tmp/icsos-posixio.log
	@echo "test-posixio PASS"

# Full QEMU integration suite for the x86_64 / SMP / ELF64 path.
test-integration: vmdex apps/hello.exe
	@echo "=== ICS-OS integration tests ==="
	$(MAKE) test-boot
	$(MAKE) test-smp
	$(MAKE) test-exec
	@echo "=== test-integration PASS ==="
