# ics-os Makefile
#
# written by Joseph Anthony C. Hermocilla
#

QEMU_I386=qemu-system-i386
QEMU_X64=qemu-system-x86_64
QEMU_MEM=128M
SMP_CPUS?=4
SMP_MATRIX?=1 2 4 8
FORK_CPUS?=2
FORK_MATRIX?=1 2 4 8
QEMU_NET=-net nic,model=rtl8139 -net none
# QEMU can load the 32-bit multiboot kernel the same way GRUB does.
QEMU_KERNEL=-kernel kernel/Kernel64.bin

vmdex:
	make -C kernel/

.PHONY: vmdex clean usb floppy livecd apps test-selfhost test-tccboot test-kbuild test-tcc-kbuild test-fullhost test-tcc-fullhost test-exec test-iobench test-virtio test-smp test-smp-matrix test-boot test-integration test-posixio test-spawn test-fork test-fork-matrix test-make test-bintools test-buildtools test-cc1 test-gcc test-gccdriver test-gcc-kbuild test-selfhost-cert test-io-unit test-vbox-usb-image test-vbox-usb-image-efi

test-io-unit:
	@mkdir -p /tmp/icsos-tests
	@gcc -std=gnu89 -Wall -Wextra -Werror -I. tests/io_p0_unit.c -o /tmp/icsos-tests/io_p0_unit
	@/tmp/icsos-tests/io_p0_unit

test-vbox-usb-image: ics-os-usb.img
	@chmod +x scripts/test-vbox-usb-image.sh
	@scripts/test-vbox-usb-image.sh ics-os-usb.img

test-vbox-usb-image-efi: ics-os-usb.img
	@chmod +x scripts/test-vbox-usb-image.sh
	@VBOX_FIRMWARE=efi scripts/test-vbox-usb-image.sh ics-os-usb.img

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
	if [ -d apps ] && ls apps/* >/dev/null 2>&1; then cp -r apps/* tmp/apps/; fi
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
	-$(MAKE) -C contrib/spawntest
	-$(MAKE) -C contrib/spawntest install
	-$(MAKE) -C contrib/forktest
	-$(MAKE) -C contrib/forktest install
	-$(MAKE) -C contrib/ext4test
	-$(MAKE) -C contrib/ext4test install
	-$(MAKE) -C contrib/sh
	-$(MAKE) -C contrib/sh install
	$(MAKE) -C contrib/tcc
	$(MAKE) -C contrib/tcc install
	$(MAKE) -C contrib/gnumake
	$(MAKE) -C contrib/gnumake install
	$(MAKE) -C contrib/buildtools
	$(MAKE) -C contrib/buildtools install
	-$(MAKE) -C contrib/bintest
	-$(MAKE) -C contrib/bintest install
	$(MAKE) -C contrib/binutils all
	$(MAKE) -C contrib/binutils install

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
		-cdrom /tmp/icsos-boot.iso -boot d < /dev/null > /tmp/icsos-boot.log 2>&1
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
	@test $(SMP_CPUS) -ge 1 && test $(SMP_CPUS) -le 8
	@echo "Running SMP $(SMP_CPUS)-CPU smoke test..."
	@status=0; timeout 60 $(QEMU_X64) -smp $(SMP_CPUS) -nographic -no-reboot -m $(QEMU_MEM) \
		-cdrom /tmp/icsos-smp.iso -boot d < /dev/null > /tmp/icsos-smp-$(SMP_CPUS).log 2>&1 || status=$$?; \
		if test $$status -ne 124; then \
			cat /tmp/icsos-smp-$(SMP_CPUS).log; exit $$status; \
		fi
	@tr -d '\r' < /tmp/icsos-smp-$(SMP_CPUS).log | \
		grep -aEq '^SMP_RESULT online=$(SMP_CPUS)$$'
	@grep -aq "Root mount \[OK\]" /tmp/icsos-smp-$(SMP_CPUS).log
	@grep -aqE "Running console thread|dex32_startup" /tmp/icsos-smp-$(SMP_CPUS).log
	@if test $(SMP_CPUS) -gt 1; then \
		expected_mask=$$(printf '%x' $$(( (1 << $(SMP_CPUS)) - 2 ))); \
		grep -aq "AP scheduling enabled" /tmp/icsos-smp-$(SMP_CPUS).log && \
		tr -d '\r' < /tmp/icsos-smp-$(SMP_CPUS).log | \
		grep -aEq "^SMP_RESULT work-steal=ok cpus=$(SMP_CPUS) mask=$${expected_mask}$$"; \
	fi
	@! grep -aqE "General Protection fault|SMP: AP work-steal (TIMEOUT|CREATE FAIL)" /tmp/icsos-smp-$(SMP_CPUS).log
	@echo "test-smp SMP_CPUS=$(SMP_CPUS) PASS"

test-smp-matrix: vmdex
	@set -e; for cpus in $(SMP_MATRIX); do \
		$(MAKE) --no-print-directory test-smp SMP_CPUS=$$cpus; \
	done
	@echo "test-smp-matrix ($(SMP_MATRIX)) PASS"

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
	@echo "Running virtio-blk test (TCG, 2 vCPUs)..."
	@-timeout 60 $(QEMU_X64) -smp 2 -nographic -no-reboot -m 256M \
		-cdrom /tmp/icsos-virtio.iso -boot d \
		-drive file=/tmp/icsos-vblk.img,if=none,id=vd0,format=raw \
		-device virtio-blk-pci,drive=vd0,disable-legacy=on \
		< /dev/null > /tmp/icsos-virtio.log 2>&1
	@grep -aq "Root mount \[OK\]" /tmp/icsos-virtio.log
	@grep -aq "VIRTIO_BLK_OK" /tmp/icsos-virtio.log
	@grep -aq "VIRTIO_IRQ_OK" /tmp/icsos-virtio.log
	@grep -aq "pipelined reads \[OK\]" /tmp/icsos-virtio.log
	@grep -aq "VIRTIO_RESET_RECOVERY_OK" /tmp/icsos-virtio.log
	@! grep -aq "VIRTIO_RESET_RECOVERY_FAIL" /tmp/icsos-virtio.log
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

# Optional TinyCC bootstrap experiment (not a supported-path gate): compile the
# kernel inside ICS-OS (C via TinyCC, GAS via prebuilt .o). The kexeced result
# currently has a known LAPIC fault. The canonical test-kbuild target uses GCC.
test-tcc-kbuild: apps vmdex
	@echo "Staging kernel sources + GAS objects for optional TinyCC kbuild..."
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
	@printf '%s\n' 'set PATH=/icsos/apps' 'tcckbuild' 'reboot' \
		> /tmp/icsos-kbuild-iso/autoexec.bat
	@printf '%s\n' 'set timeout=0' 'menuentry "ics" { multiboot2 /vmdex; boot }' \
		> /tmp/icsos-kbuild-iso/boot/grub/grub.cfg
	@grub-mkrescue -o /tmp/icsos-kbuild.iso /tmp/icsos-kbuild-iso >/dev/null 2>&1
	@echo "Running optional TinyCC kbuild test (KVM)..."
	@-timeout 3600 stdbuf -oL $(QEMU_X64) -enable-kvm -cpu host -smp 1 -nographic -no-reboot -m 1024M \
		-cdrom /tmp/icsos-kbuild.iso -boot d < /dev/null > /tmp/icsos-kbuild.log 2>&1
	@grep -aq "Root mount \[OK\]" /tmp/icsos-kbuild.log
	@grep -aq "KBUILD_TEST_PASS" /tmp/icsos-kbuild.log
	@grep -aq "KBUILD_KEXEC" /tmp/icsos-kbuild.log
	@grep -aq "Build: kbuild-inos" /tmp/icsos-kbuild.log
	@grep -aq "KEXEC_BOOT_OK" /tmp/icsos-kbuild.log
	@! grep -aq "KBUILD_TEST_FAIL" /tmp/icsos-kbuild.log
	@echo "test-tcc-kbuild PASS"

# Optional TinyCC bootstrap: compile TinyCC with TinyCC, then compile the
# kernel with tccnew. GCC remains the supported self-host compiler.
test-tcc-fullhost: apps vmdex
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
	@printf '%s\n' 'set PATH=/icsos/apps' 'tccfullhost' 'reboot' \
		> /tmp/icsos-fullhost-iso/autoexec.bat
	@printf '%s\n' 'set timeout=0' 'menuentry "ics" { multiboot2 /vmdex; boot }' \
		> /tmp/icsos-fullhost-iso/boot/grub/grub.cfg
	@grub-mkrescue -o /tmp/icsos-fullhost.iso /tmp/icsos-fullhost-iso >/dev/null 2>&1
	@echo "Running fullhost test (KVM, long)..."
	@-timeout 3600 $(QEMU_X64) -enable-kvm -cpu host -smp 1 -nographic -no-reboot -m 1024M \
		-cdrom /tmp/icsos-fullhost.iso -boot d < /dev/null > /tmp/icsos-fullhost.log 2>&1
	@grep -aq "TCCBOOT_TEST_PASS" /tmp/icsos-fullhost.log
	@grep -aq "KBUILD_TEST_PASS" /tmp/icsos-fullhost.log
	@grep -aq "KBUILD_KEXEC" /tmp/icsos-fullhost.log
	@grep -aq "Build: fullhost-inos" /tmp/icsos-fullhost.log
	@grep -aq "KEXEC_BOOT_OK" /tmp/icsos-fullhost.log
	@grep -aq "FULLHOST_TEST_PASS" /tmp/icsos-fullhost.log
	@echo "test-tcc-fullhost PASS"

# Generic self-host names always select the supported GCC path. The compiler
# bootstrap experiment is available only through test-tcc-fullhost.
test-fullhost: test-kbuild
	@echo "test-fullhost PASS (GCC)"
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
		-cdrom /tmp/icsos-exec.iso -boot d < /dev/null > /tmp/icsos-exec.log 2>&1
	@grep -aq "Root mount \[OK\]" /tmp/icsos-exec.log
	@grep -aq "exectest: running" /tmp/icsos-exec.log
	@grep -aq "Hello World from ICS-OS!" /tmp/icsos-exec.log
	@(grep -aq "EXEC_TEST_PASS" /tmp/icsos-exec.log || \
	  (grep -aq "Hello World from ICS-OS!" /tmp/icsos-exec.log && \
	   ! grep -aq "EXEC_TEST_FAIL" /tmp/icsos-exec.log))
	@echo "test-exec PASS"

# POSIX fds, VFS create cleanup, preadv/pwritev/fsync, io_uring, and async /dev/vblk.
test-posixio: kernel/Kernel64.bin apps/posixio.exe
	@echo "Building Multiboot2 ISO for POSIX I/O..."
	@mkdir -p /tmp/icsos-posixio-iso/boot/grub /tmp/icsos-posixio-iso/apps
	@cp kernel/Kernel64.bin /tmp/icsos-posixio-iso/vmdex
	@cp apps/posixio.exe /tmp/icsos-posixio-iso/apps/posixio.exe
	@printf '%s\n' 'posixio' 'posixio' 'reboot' > /tmp/icsos-posixio-iso/autoexec.bat
	@printf '%s\n' 'set timeout=0' 'menuentry "ics" { multiboot2 /vmdex; boot }' \
		> /tmp/icsos-posixio-iso/boot/grub/grub.cfg
	@grub-mkrescue -o /tmp/icsos-posixio.iso /tmp/icsos-posixio-iso >/dev/null 2>&1
	@echo "Running POSIX I/O test (TCG, 2 vCPUs, virtio-blk)..."
	@dd if=/dev/zero of=/tmp/icsos-posixio-vblk.img bs=1M count=8 status=none
	@-timeout 70 $(QEMU_X64) -smp 2 -nographic -no-reboot -m 256M \
		-cdrom /tmp/icsos-posixio.iso -boot d \
		-drive file=/tmp/icsos-posixio-vblk.img,if=none,id=vd0,format=raw \
		-device virtio-blk-pci,drive=vd0,disable-legacy=on \
		< /dev/null > /tmp/icsos-posixio.log 2>&1
	@grep -aq "Root mount \[OK\]" /tmp/icsos-posixio.log
	@grep -aq "ramdisk: mounted" /tmp/icsos-posixio.log
	@grep -aq "POSIXIO_PASS" /tmp/icsos-posixio.log
	@grep -aq "URING_PASS" /tmp/icsos-posixio.log
	@grep -aq "URING_VBLK_PASS" /tmp/icsos-posixio.log
	@test "$$(grep -ac "URING_CLOSE_DRAIN_PASS" /tmp/icsos-posixio.log)" -eq 2
	@test "$$(grep -ac "VFS_CREATE_UNLOCK_PASS" /tmp/icsos-posixio.log)" -eq 2
	@test "$$(grep -ac "POSIXIO_PASS" /tmp/icsos-posixio.log)" -eq 2
	@! grep -aq "POSIXIO_FAIL" /tmp/icsos-posixio.log
	@echo "test-posixio PASS"

# posix_spawn + waitpid of hello.exe, plus FAT /work on virtio-blk.
test-spawn: kernel/Kernel64.bin apps/spawn.exe apps/hello.exe
	@$(MAKE) -C kernel all
	@echo "Building Multiboot2 ISO for spawn + work disk..."
	@mkdir -p /tmp/icsos-spawn-iso/boot/grub /tmp/icsos-spawn-iso/apps
	@cp kernel/Kernel64.bin /tmp/icsos-spawn-iso/vmdex
	@cp apps/spawn.exe /tmp/icsos-spawn-iso/apps/spawn.exe
	@cp apps/hello.exe /tmp/icsos-spawn-iso/apps/hello.exe
	@printf '%s\n' 'spawntest' 'reboot' > /tmp/icsos-spawn-iso/autoexec.bat
	@printf '%s\n' 'set timeout=0' 'menuentry "ics" { multiboot2 /vmdex; boot }' \
		> /tmp/icsos-spawn-iso/boot/grub/grub.cfg
	@grub-mkrescue -o /tmp/icsos-spawn.iso /tmp/icsos-spawn-iso >/dev/null 2>&1
	@echo "Formatting 512 MiB FAT16 virtio work disk..."
	@truncate -s 512M /tmp/icsos-work.img
	@mkfs.vfat -F 16 -s 32 /tmp/icsos-work.img >/dev/null
	@echo "Running spawn test (KVM, virtio-blk)..."
	@-timeout 70 stdbuf -oL $(QEMU_X64) -enable-kvm -cpu host -smp 1 -nographic -no-reboot -m 256M \
		-cdrom /tmp/icsos-spawn.iso -boot d \
		-drive file=/tmp/icsos-work.img,if=none,id=vd0,format=raw \
		-device virtio-blk-pci,drive=vd0,disable-legacy=on \
		< /dev/null > /tmp/icsos-spawn.log 2>&1
	@grep -aq "Root mount \[OK\]" /tmp/icsos-spawn.log
	@grep -aq "work: mounted" /tmp/icsos-spawn.log
	@grep -aq "SPAWN_PASS" /tmp/icsos-spawn.log
	@grep -aq "FD_INHERIT_CHILD_PASS" /tmp/icsos-spawn.log
	@grep -aq "FD_INHERIT_PASS" /tmp/icsos-spawn.log
	@grep -aq "WORK_DISK_PASS" /tmp/icsos-spawn.log
	@! grep -aq "SPAWN_FAIL" /tmp/icsos-spawn.log
	@! grep -aq "FD_INHERIT_FAIL" /tmp/icsos-spawn.log
	@! grep -aq "WORK_DISK_FAIL" /tmp/icsos-spawn.log
	@echo "test-spawn PASS"

apps/forktest.exe: contrib/forktest/forktest.c contrib/forktest/Makefile
	$(MAKE) -C contrib/forktest install

# COW fork: return values, shared-page writes, fd inheritance, exit/wait.
test-fork: kernel/Kernel64.bin apps/forktest.exe
	@$(MAKE) -C kernel all
	@echo "Building Multiboot2 ISO for fork test..."
	@rm -rf /tmp/icsos-fork-iso
	@mkdir -p /tmp/icsos-fork-iso/boot/grub /tmp/icsos-fork-iso/apps
	@cp kernel/Kernel64.bin /tmp/icsos-fork-iso/vmdex
	@cp apps/forktest.exe /tmp/icsos-fork-iso/apps/forktest.exe
	@printf '%s\n' 'forktest' 'forkoomtest' 'reboot' > /tmp/icsos-fork-iso/autoexec.bat
	@printf '%s\n' 'set timeout=0' 'menuentry "ics" { multiboot2 /vmdex; boot }' \
		> /tmp/icsos-fork-iso/boot/grub/grub.cfg
	@grub-mkrescue -o /tmp/icsos-fork.iso /tmp/icsos-fork-iso >/dev/null 2>&1
	@truncate -s 64M /tmp/icsos-fork-work.img
	@mkfs.vfat -F 16 /tmp/icsos-fork-work.img >/dev/null
	@test $(FORK_CPUS) -ge 1 && test $(FORK_CPUS) -le 8
	@echo "Running COW fork test (TCG, $(FORK_CPUS) vCPUs)..."
	@status=0; timeout 70 $(QEMU_X64) -smp $(FORK_CPUS) -nographic -no-reboot -m 256M \
		-cdrom /tmp/icsos-fork.iso -boot d \
		-drive file=/tmp/icsos-fork-work.img,if=none,id=vd0,format=raw \
		-device virtio-blk-pci,drive=vd0,disable-legacy=on \
		< /dev/null > /tmp/icsos-fork.log 2>&1 || status=$$?; \
		if test $$status -ne 0; then cat /tmp/icsos-fork.log; exit $$status; fi
	@grep -aq "Root mount \[OK\]" /tmp/icsos-fork.log
	@grep -aq "work: mounted" /tmp/icsos-fork.log
	@grep -aq "FORK_CHILD_PASS" /tmp/icsos-fork.log
	@grep -aq "FORK_PARENT_PASS" /tmp/icsos-fork.log
	@grep -aq "FORK_COW_PASS" /tmp/icsos-fork.log
	@grep -aq "FORK_TEXT_FAULT_PASS" /tmp/icsos-fork.log
	@grep -aq "FORK_COW_OOM_PASS" /tmp/icsos-fork.log
	@grep -aq "COW_FAULT copy" /tmp/icsos-fork.log
	@grep -aq "COW_FAULT fast" /tmp/icsos-fork.log
	@grep -aq "COW_FAULT oom-injected" /tmp/icsos-fork.log
	@grep -aq "FORK_STRESS_PASS count=10" /tmp/icsos-fork.log
	@grep -aq "FORK_PASS" /tmp/icsos-fork.log
	@! grep -aq "FORK_FAIL\|FORK_RUN_FAIL" /tmp/icsos-fork.log
	@! grep -aq "General Protection Fault\|DOUBLE FAULT" /tmp/icsos-fork.log
	@test $$(grep -ac "Page fault (" /tmp/icsos-fork.log) -eq 1
	@echo "test-fork FORK_CPUS=$(FORK_CPUS) PASS"

test-fork-matrix: kernel/Kernel64.bin apps/forktest.exe
	@set -e; for cpus in $(FORK_MATRIX); do \
		$(MAKE) --no-print-directory test-fork FORK_CPUS=$$cpus; \
	done
	@echo "test-fork-matrix ($(FORK_MATRIX)) PASS"

apps/ext4test.exe:
	$(MAKE) -C contrib/ext4test install

# ext4 driver: read host-seeded file + create/write/mkdir on /work (ext4 on virtio-blk).
test-ext4: kernel/Kernel64.bin apps/ext4test.exe
	@$(MAKE) -C kernel all
	@echo "Building Multiboot2 ISO for ext4 test..."
	@mkdir -p /tmp/icsos-ext4-iso/boot/grub /tmp/icsos-ext4-iso/apps
	@cp kernel/Kernel64.bin /tmp/icsos-ext4-iso/vmdex
	@cp apps/ext4test.exe /tmp/icsos-ext4-iso/apps/ext4test.exe
	@printf '%s\n' 'ext4test' 'reboot' > /tmp/icsos-ext4-iso/autoexec.bat
	@printf '%s\n' 'set timeout=0' 'menuentry "ics" { multiboot2 /vmdex; boot }' \
		> /tmp/icsos-ext4-iso/boot/grub/grub.cfg
	@grub-mkrescue -o /tmp/icsos-ext4.iso /tmp/icsos-ext4-iso >/dev/null 2>&1
	@echo "Creating 64 MiB ext4 virtio work disk + seeding /seed.txt..."
	@truncate -s 64M /tmp/icsos-ext4-work.img
	@mkfs.ext4 -F -q -b 4096 -O ^has_journal -E lazy_itable_init=0 /tmp/icsos-ext4-work.img
	@printf '%s' 'icsos-ext4-seed' > /tmp/icsos-ext4-seed.txt
	@debugfs -w -R "write /tmp/icsos-ext4-seed.txt /seed.txt" /tmp/icsos-ext4-work.img >/dev/null 2>&1
	@echo "Running ext4 test (TCG, virtio-blk)..."
	@-timeout 90 stdbuf -oL $(QEMU_X64) -nographic -no-reboot -m 256M \
		-cdrom /tmp/icsos-ext4.iso -boot d \
		-drive file=/tmp/icsos-ext4-work.img,if=none,id=vd0,format=raw \
		-device virtio-blk-pci,drive=vd0,disable-legacy=on \
		< /dev/null > /tmp/icsos-ext4.log 2>&1
	@grep -aq "Root mount \[OK\]" /tmp/icsos-ext4.log
	@grep -aq "work: mounted ext4" /tmp/icsos-ext4.log
	@grep -aq "EXT4_READ_PASS" /tmp/icsos-ext4.log
	@grep -aq "EXT4_WRITE_PASS" /tmp/icsos-ext4.log
	@grep -aq "EXT4_DIR_PASS" /tmp/icsos-ext4.log
	@grep -aq "EXT4_PASS" /tmp/icsos-ext4.log
	@! grep -aq "EXT4_READ_FAIL" /tmp/icsos-ext4.log
	@! grep -aq "EXT4_WRITE_FAIL" /tmp/icsos-ext4.log
	@! grep -aq "EXT4_DIR_FAIL" /tmp/icsos-ext4.log
	@! grep -aq "EXT4_RUN_FAIL" /tmp/icsos-ext4.log
	@echo "test-ext4 PASS"

# Artifacts for test-bintools (file rules so the target works standalone).
apps/bintest.exe:
	$(MAKE) -C contrib/bintest install
apps/as.exe apps/ld.exe apps/ar.exe:
	$(MAKE) -C contrib/binutils all
	$(MAKE) -C contrib/binutils install
apps/gccboot.exe:
	$(MAKE) -C contrib/gccboot install

# In-OS binutils: run as/ar/ld inside the OS (posix_spawn + waitpid), verify
# their output files (the kernel does not propagate a child's exit status).
# autoexec.bat must name "bintest.exe" explicitly: the console's generic
# "not a builtin" path maps $PATH/<name> verbatim and does NOT append ".exe".
# ld is invoked WITHOUT -T: it must find its default script in
# /icsos/apps/ldscripts/ (staged next to the binary) — the same mechanism the
# later GCC self-host depends on. Intermediates live on /ramdisk (16 MiB RAM),
# so no virtio /work disk is needed.
test-bintools: kernel/Kernel64.bin apps/bintest.exe apps/as.exe apps/ld.exe apps/ar.exe
	@echo "Building Multiboot2 ISO for in-OS binutils test..."
	@rm -rf /tmp/icsos-bintools-iso
	@mkdir -p /tmp/icsos-bintools-iso/boot/grub /tmp/icsos-bintools-iso/apps/ldscripts
	@cp kernel/Kernel64.bin /tmp/icsos-bintools-iso/vmdex
	@cp apps/bintest.exe /tmp/icsos-bintools-iso/apps/bintest.exe
	@cp apps/as.exe /tmp/icsos-bintools-iso/apps/as.exe
	@cp apps/ld.exe /tmp/icsos-bintools-iso/apps/ld.exe
	@cp apps/ar.exe /tmp/icsos-bintools-iso/apps/ar.exe
	@cp apps/ldscripts/elf_x86_64.x* /tmp/icsos-bintools-iso/apps/ldscripts/
	@cp contrib/bintest/mini.s /tmp/icsos-bintools-iso/mini.s
	@printf '%s\n' 'bintest.exe' 'reboot' > /tmp/icsos-bintools-iso/autoexec.bat
	@printf '%s\n' 'set timeout=0' 'menuentry "ics" { multiboot2 /vmdex; boot }' \
		> /tmp/icsos-bintools-iso/boot/grub/grub.cfg
	@grub-mkrescue -o /tmp/icsos-bintools.iso /tmp/icsos-bintools-iso >/dev/null 2>&1
	@echo "Running in-OS binutils test (KVM)..."
	@-timeout 90 stdbuf -oL $(QEMU_X64) -enable-kvm -cpu host -smp 1 -nographic -no-reboot -m 256M \
		-cdrom /tmp/icsos-bintools.iso -boot d \
		< /dev/null > /tmp/icsos-bintools.log 2>&1
	@grep -aq "Root mount \[OK\]" /tmp/icsos-bintools.log
	@grep -aq "AS_PASS" /tmp/icsos-bintools.log
	@grep -aq "AR_PASS" /tmp/icsos-bintools.log
	@grep -aq "LD_PASS" /tmp/icsos-bintools.log
	@grep -aq "LD_EXEC_PASS" /tmp/icsos-bintools.log
	@grep -aq "BINTOOLS_PASS" /tmp/icsos-bintools.log
	@! grep -aq "AS_FAIL\|AR_FAIL\|LD_FAIL\|LD_EXEC_FAIL" /tmp/icsos-bintools.log
	@echo "test-bintools PASS"

# In-OS TinyCC compiles GNU make 3.82 onto FAT /work, then make posix_spawns hello.exe.
apps/make.exe:
	$(MAKE) -C contrib/gnumake install

apps/cp.exe apps/mkdir.exe apps/rm.exe:
	$(MAKE) -C contrib/buildtools install

apps/objcopy.exe:
	$(MAKE) -C contrib/binutils objcopy install

test-buildtools: vmdex apps/cp.exe apps/mkdir.exe apps/rm.exe apps/objcopy.exe
	@$(MAKE) -C contrib/buildtools install
	@echo "Building Multiboot2 ISO for native build-tool test..."
	@rm -rf /tmp/icsos-buildtools-iso
	@mkdir -p /tmp/icsos-buildtools-iso/boot/grub /tmp/icsos-buildtools-iso/apps
	@cp kernel/Kernel64.bin /tmp/icsos-buildtools-iso/vmdex
	@cp apps/cp.exe apps/mkdir.exe apps/rm.exe apps/objcopy.exe apps/buildtest.exe \
		/tmp/icsos-buildtools-iso/apps/
	@printf '%s\n' 'buildtest.exe' 'reboot' > /tmp/icsos-buildtools-iso/autoexec.bat
	@printf '%s\n' 'set timeout=0' 'menuentry "ics" { multiboot2 /vmdex; boot }' \
		> /tmp/icsos-buildtools-iso/boot/grub/grub.cfg
	@grub-mkrescue -o /tmp/icsos-buildtools.iso /tmp/icsos-buildtools-iso >/dev/null 2>&1
	@echo "Running native build-tool test (KVM)..."
	@-timeout 180 $(QEMU_X64) -enable-kvm -cpu host -smp 1 -nographic -no-reboot -m 512M \
		-cdrom /tmp/icsos-buildtools.iso -boot d < /dev/null \
		> /tmp/icsos-buildtools.log 2>&1
	@grep -aq "BUILDTOOLS_CP_MKDIR_OK" /tmp/icsos-buildtools.log
	@grep -aq "BUILDTOOLS_OBJCOPY_OK" /tmp/icsos-buildtools.log
	@grep -aq "BUILDTOOLS_RM_OK" /tmp/icsos-buildtools.log
	@grep -aq "BUILDTOOLS_TEST_PASS" /tmp/icsos-buildtools.log
	@! grep -aq "BUILDTOOLS_TEST_FAIL\|General Protection fault" /tmp/icsos-buildtools.log
	@echo "test-buildtools PASS"

test-make: vmdex apps/tcc.exe apps/hello.exe apps/make.exe
	@echo "Staging GNU make sources + sdkobj (host tcc -E)..."
	@chmod +x scripts/stage-make.sh scripts/stage-make-short.sh
	@scripts/stage-make-short.sh /tmp/icsos-make-short
	@tar --format=ustar -C /tmp/icsos-make-short -cf /tmp/icsos-makesrc.tar .
	@echo "Building Multiboot2 ISO for makeboot..."
	@rm -rf /tmp/icsos-make-iso
	@mkdir -p /tmp/icsos-make-iso/boot/grub /tmp/icsos-make-iso/apps
	@cp kernel/Kernel64.bin /tmp/icsos-make-iso/vmdex
	@cp apps/tcc.exe /tmp/icsos-make-iso/apps/tcc.exe
	@cp apps/hello.exe /tmp/icsos-make-iso/apps/hello.exe
	@cp apps/make.exe /tmp/icsos-make-iso/apps/make.exe
	@cp /tmp/icsos-makesrc.tar /tmp/icsos-make-iso/makesrc.tar
	@printf '%s\n' 'set PATH=/icsos/apps' 'makeboot' 'reboot' \
		> /tmp/icsos-make-iso/autoexec.bat
	@printf '%s\n' 'set timeout=0' 'menuentry "ics" { multiboot2 /vmdex; boot }' \
		> /tmp/icsos-make-iso/boot/grub/grub.cfg
	@grub-mkrescue -o /tmp/icsos-make.iso /tmp/icsos-make-iso >/dev/null 2>&1
	@echo "Formatting 512 MiB FAT16 virtio work disk..."
	@truncate -s 512M /tmp/icsos-make-work.img
	@mkfs.vfat -F 16 -s 32 /tmp/icsos-make-work.img >/dev/null
	@echo "Running makeboot test (KVM, virtio-blk)..."
	@-timeout 1800 stdbuf -oL $(QEMU_X64) -enable-kvm -cpu host -smp 1 -nographic -no-reboot -m 1024M \
		-cdrom /tmp/icsos-make.iso -boot d \
		-drive file=/tmp/icsos-make-work.img,if=none,id=vd0,format=raw \
		-device virtio-blk-pci,drive=vd0,disable-legacy=on \
		> /tmp/icsos-make.log 2>&1
	@grep -aq "Root mount \[OK\]" /tmp/icsos-make.log
	@grep -aq "work: mounted" /tmp/icsos-make.log
	@grep -aq "MAKE_TCC_OK" /tmp/icsos-make.log
	@grep -aq "Hello World from ICS-OS!" /tmp/icsos-make.log
	@grep -aq "MAKE_PASS" /tmp/icsos-make.log
	@! grep -aq "MAKE_FAIL" /tmp/icsos-make.log
	@echo "test-make PASS"

# In-OS GCC cc1 compile test: run the host-built cc1 (a full GCC C frontend,
# ~18MB) to compile a trivial C file to assembly, proving the GCC frontend
# runs on ICS-OS. cc1 runs from the CD (too large for the 16MiB ramdisk).
CC1_PROBE ?= /tmp/opencode/ggcstress1000.c
CC1_TIMEOUT ?= 900
test-cc1: kernel/Kernel64.bin
	@if [ ! -s /tmp/icsos-gcc/cc1 ]; then \
	   echo "Building host cc1 (contrib/gcc)..." ; \
	   $(MAKE) -C contrib/gcc cc1 ; \
	fi
	@echo "Building Multiboot2 ISO for in-OS GCC cc1 compile test..."
	@rm -rf /tmp/icsos-cc1-iso
	@mkdir -p /tmp/icsos-cc1-iso/boot/grub /tmp/icsos-cc1-iso/apps
	@cp kernel/Kernel64.bin /tmp/icsos-cc1-iso/vmdex
	@cp /tmp/icsos-gcc/cc1 /tmp/icsos-cc1-iso/apps/cc1.exe
	@cp $(CC1_PROBE) /tmp/icsos-cc1-iso/apps/cc1probe.c
	@printf '%s\n' 'set PATH=/icsos/apps' 'cc1test' 'reboot' > /tmp/icsos-cc1-iso/autoexec.bat
	@printf '%s\n' 'set timeout=0' 'menuentry "ics" { multiboot2 /vmdex; boot }' \
		> /tmp/icsos-cc1-iso/boot/grub/grub.cfg
	@grub-mkrescue -o /tmp/icsos-cc1.iso /tmp/icsos-cc1-iso >/dev/null 2>&1
	@echo "Running in-OS GCC cc1 compile test (KVM)..."
	@-timeout $(CC1_TIMEOUT) stdbuf -oL $(QEMU_X64) -enable-kvm -cpu host -smp 1 -nographic -no-reboot -m 1024M \
		-cdrom /tmp/icsos-cc1.iso -boot d > /tmp/icsos-cc1.log 2>&1
	@grep -aq "Root mount \[OK\]" /tmp/icsos-cc1.log
	@grep -aq "cc1test:" /tmp/icsos-cc1.log
	@grep -aq "CC1_TEST_PASS" /tmp/icsos-cc1.log
	@! grep -aq "CC1_TEST_FAIL" /tmp/icsos-cc1.log
	@echo "test-cc1 PASS"

# In-OS full GCC toolchain test: cc1 (C frontend) -> as (GAS) -> ld (GNU ld)
# -> exec. Proves the real GCC C frontend and the real binutils backend compose
# in-OS to BUILD AND RUN a C program (the GCC self-host capstone step 8).
# cc1 runs from the CD (too large for the ramdisk); as/ld + the SDK runtime
# objects + the linker scripts are staged on the CD; all intermediates live on
# /ramdisk (so the spawned tool children never read the CD mid-run).
test-gcc: kernel/Kernel64.bin apps/as.exe apps/ld.exe
	@if [ ! -s /tmp/icsos-gcc/cc1 ]; then \
	   echo "Building host cc1 (contrib/gcc)..." ; \
	   $(MAKE) -C contrib/gcc cc1 ; \
	fi
	@echo "Building SDK runtime objects for the in-OS ld step..."
	@mkdir -p /tmp/icsos-gcc/rt
	@gcc -m64 -std=gnu89 -w -nostdlib -fno-builtin -static -ffreestanding -fno-pie -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables -fno-strict-aliasing -mcmodel=large -mno-red-zone -Isdk/include -c sdk/crt1.c -o /tmp/icsos-gcc/rt/crt1.o
	@gcc -m64 -std=gnu89 -w -nostdlib -fno-builtin -static -ffreestanding -fno-pie -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables -fno-strict-aliasing -mcmodel=large -mno-red-zone -Isdk/include -c sdk/tccsdk.c -o /tmp/icsos-gcc/rt/tccsdk.o
	@gcc -m64 -std=gnu89 -w -nostdlib -fno-builtin -static -ffreestanding -fno-pie -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables -fno-strict-aliasing -mcmodel=large -mno-red-zone -Isdk/include -c sdk/libtcc1.c -o /tmp/icsos-gcc/rt/libtcc1.o
	@gcc -m64 -std=gnu89 -w -nostdlib -fno-builtin -static -ffreestanding -fno-pie -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables -fno-strict-aliasing -mcmodel=large -mno-red-zone -Isdk/include -c sdk/posix.c -o /tmp/icsos-gcc/rt/posix.o
	@gcc -m64 -std=gnu89 -w -nostdlib -fno-builtin -static -ffreestanding -fno-pie -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables -fno-strict-aliasing -mcmodel=large -mno-red-zone -Isdk/include -c sdk/setjmp.c -o /tmp/icsos-gcc/rt/setjmp.o
	@echo "Building Multiboot2 ISO for in-OS GCC toolchain (cc1 + as + ld) test..."
	@rm -rf /tmp/icsos-gcc-iso
	@mkdir -p /tmp/icsos-gcc-iso/boot/grub /tmp/icsos-gcc-iso/apps/ldscripts
	@cp kernel/Kernel64.bin /tmp/icsos-gcc-iso/vmdex
	@cp /tmp/icsos-gcc/cc1 /tmp/icsos-gcc-iso/apps/cc1.exe
	@cp apps/as.exe /tmp/icsos-gcc-iso/apps/as.exe
	@cp apps/ld.exe /tmp/icsos-gcc-iso/apps/ld.exe
	@cp /tmp/icsos-gcc/rt/crt1.o /tmp/icsos-gcc/rt/tccsdk.o /tmp/icsos-gcc/rt/libtcc1.o /tmp/icsos-gcc/rt/posix.o /tmp/icsos-gcc/rt/setjmp.o /tmp/icsos-gcc-iso/apps/
	@cp apps/ldscripts/elf_x86_64.x* /tmp/icsos-gcc-iso/apps/ldscripts/
	@printf 'extern int puts(const char *s);\nint main(void)\n{\n   puts("GCC_E2E_OK");\n   return 0;\n}\n' > /tmp/icsos-gcc-iso/apps/gccprobe.c
	@printf '%s\n' 'set PATH=/icsos/apps' 'gctest' 'reboot' > /tmp/icsos-gcc-iso/autoexec.bat
	@printf '%s\n' 'set timeout=0' 'menuentry "ics" { multiboot2 /vmdex; boot }' \
		> /tmp/icsos-gcc-iso/boot/grub/grub.cfg
	@grub-mkrescue -o /tmp/icsos-gcc.iso /tmp/icsos-gcc-iso >/dev/null 2>&1
	@echo "Running in-OS GCC toolchain test (cc1 -> as -> ld -> exec) (KVM)..."
	@-timeout 420 stdbuf -oL $(QEMU_X64) -enable-kvm -cpu host -smp 1 -nographic -no-reboot -m 1024M \
		-cdrom /tmp/icsos-gcc.iso -boot d > /tmp/icsos-gcc.log 2>&1
	@grep -aq "Root mount \[OK\]" /tmp/icsos-gcc.log
	@grep -aq "gctest:" /tmp/icsos-gcc.log
	@grep -aq "GCC_E2E_OK" /tmp/icsos-gcc.log
	@grep -aq "GCC_E2E_RUN_OK" /tmp/icsos-gcc.log
	@! grep -aq "GCC_E2E_FAIL" /tmp/icsos-gcc.log
	@echo "test-gcc PASS"

# In-OS gcc DRIVER test: the standalone gcc.exe (contrib/gccdriver) is a real
# user program that spawns cc1/as/ld itself via posix_spawn and links a C
# program, which we then exec. Unlike test-gcc (the kernel console builtin
# chains the tools), this proves the gcc front-end DRIVER works as a user
# program -- the glue the self-host needs (a gcc that can be invoked and that
# can itself be built by gcc). Same CD/ramdisk staging as test-gcc.
test-gccdriver: kernel/Kernel64.bin apps/as.exe apps/ld.exe
	@if [ ! -s /tmp/icsos-gcc/cc1 ]; then \
	   echo "Building host cc1 (contrib/gcc)..." ; \
	   $(MAKE) -C contrib/gcc cc1 ; \
	fi
	@if [ ! -s contrib/gccdriver/gcc.exe ]; then \
	   echo "Building gcc driver (contrib/gccdriver)..." ; \
	   $(MAKE) -C contrib/gccdriver ; \
	fi
	@echo "Building SDK runtime objects for the in-OS ld step..."
	@mkdir -p /tmp/icsos-gcc/rt
	@gcc -m64 -std=gnu89 -w -nostdlib -fno-builtin -static -ffreestanding -fno-pie -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables -fno-strict-aliasing -mcmodel=large -mno-red-zone -Isdk/include -c sdk/crt1.c -o /tmp/icsos-gcc/rt/crt1.o
	@gcc -m64 -std=gnu89 -w -nostdlib -fno-builtin -static -ffreestanding -fno-pie -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables -fno-strict-aliasing -mcmodel=large -mno-red-zone -Isdk/include -c sdk/tccsdk.c -o /tmp/icsos-gcc/rt/tccsdk.o
	@gcc -m64 -std=gnu89 -w -nostdlib -fno-builtin -static -ffreestanding -fno-pie -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables -fno-strict-aliasing -mcmodel=large -mno-red-zone -Isdk/include -c sdk/libtcc1.c -o /tmp/icsos-gcc/rt/libtcc1.o
	@gcc -m64 -std=gnu89 -w -nostdlib -fno-builtin -static -ffreestanding -fno-pie -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables -fno-strict-aliasing -mcmodel=large -mno-red-zone -Isdk/include -c sdk/posix.c -o /tmp/icsos-gcc/rt/posix.o
	@gcc -m64 -std=gnu89 -w -nostdlib -fno-builtin -static -ffreestanding -fno-pie -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables -fno-strict-aliasing -mcmodel=large -mno-red-zone -Isdk/include -c sdk/setjmp.c -o /tmp/icsos-gcc/rt/setjmp.o
	@echo "Building Multiboot2 ISO for in-OS gcc driver test..."
	@rm -rf /tmp/icsos-gccdrv-iso
	@mkdir -p /tmp/icsos-gccdrv-iso/boot/grub /tmp/icsos-gccdrv-iso/apps/ldscripts
	@cp kernel/Kernel64.bin /tmp/icsos-gccdrv-iso/vmdex
	@cp /tmp/icsos-gcc/cc1 /tmp/icsos-gccdrv-iso/apps/cc1.exe
	@cp apps/as.exe /tmp/icsos-gccdrv-iso/apps/as.exe
	@cp apps/ld.exe /tmp/icsos-gccdrv-iso/apps/ld.exe
	@cp contrib/gccdriver/gcc.exe /tmp/icsos-gccdrv-iso/apps/gcc.exe
	@cp /tmp/icsos-gcc/rt/crt1.o /tmp/icsos-gcc/rt/tccsdk.o /tmp/icsos-gcc/rt/libtcc1.o /tmp/icsos-gcc/rt/posix.o /tmp/icsos-gcc/rt/setjmp.o /tmp/icsos-gccdrv-iso/apps/
	@cp apps/ldscripts/elf_x86_64.x* /tmp/icsos-gccdrv-iso/apps/ldscripts/
	@printf 'extern int puts(const char *s);\nint main(void)\n{\n   puts("GCC_DRV_OK");\n   return 0;\n}\n' > /tmp/icsos-gccdrv-iso/apps/drvprobe.c
	@printf '%s\n' 'set PATH=/icsos/apps' 'gccdrv' 'reboot' > /tmp/icsos-gccdrv-iso/autoexec.bat
	@printf '%s\n' 'set timeout=0' 'menuentry "ics" { multiboot2 /vmdex; boot }' \
		> /tmp/icsos-gccdrv-iso/boot/grub/grub.cfg
	@grub-mkrescue -o /tmp/icsos-gccdrv.iso /tmp/icsos-gccdrv-iso >/dev/null 2>&1
	@echo "Running in-OS gcc driver test (gcc -> cc1/as/ld -> exec) (KVM)..."
	@rm -f /tmp/icsos-gccdrv.log
	@-timeout 420 $(QEMU_X64) -enable-kvm -cpu host -smp 1 -display none -monitor none \
		-serial file:/tmp/icsos-gccdrv.log -no-reboot -m 1024M \
		-cdrom /tmp/icsos-gccdrv.iso -boot d < /dev/null
	@grep -aq "Root mount \[OK\]" /tmp/icsos-gccdrv.log
	@grep -aq "gccdrv:" /tmp/icsos-gccdrv.log
	@grep -aq "GCC_DRIVER_OK" /tmp/icsos-gccdrv.log
	@grep -aq "GCC_DRV_OK" /tmp/icsos-gccdrv.log
	@grep -aq "GCC_DRV_RUN_OK" /tmp/icsos-gccdrv.log
	@! grep -aq "GCC_DRV_FAIL" /tmp/icsos-gccdrv.log
	@echo "test-gccdriver PASS"

# In-OS GCC KERNEL build test (self-host capstone): the in-OS gcc driver
# (gcc -> cc1 -> as) compiles ALL kernel C + ASM with the EXACT host kernel
# CFLAGS and in-OS ld links the kernel with the host LDFLAGS + lscript64.ld,
# mirroring `make -C kernel`. The in-OS-built kernel is kexec'd; the kexeced
# kernel prints KEXEC_BOOT_OK and reboots (QEMU -no-reboot exits).
test-gcc-kbuild: vmdex apps/as.exe apps/ld.exe apps/hello.exe apps/make.exe apps/objcopy.exe apps/cp.exe
	@if [ ! -s /tmp/icsos-gcc/cc1 ]; then \
	   echo "Building host cc1 (contrib/gcc)..." ; \
	   $(MAKE) -C contrib/gcc cc1 ; \
	fi
	@if [ ! -s contrib/gccdriver/gcc.exe ]; then \
	   echo "Building gcc driver (contrib/gccdriver)..." ; \
	   $(MAKE) -C contrib/gccdriver ; \
	fi
	@echo "Building SDK runtime objects for post-kexec GCC verification..."
	@mkdir -p /tmp/icsos-gcc/rt
	@for src in crt1 tccsdk libtcc1 posix setjmp; do \
	   gcc -m64 -std=gnu89 -w -nostdlib -fno-builtin -static -ffreestanding \
	      -fno-pie -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables \
	      -fno-strict-aliasing -mcmodel=large -mno-red-zone -Isdk/include \
	      -c sdk/$$src.c -o /tmp/icsos-gcc/rt/$$src.o || exit 1; \
	done
	@echo "Staging kernel sources for in-OS gcc kbuild..."
	@scripts/stage-kernel-src.sh /tmp/icsos-gkbuild
	@echo "Building Multiboot2 ISO for in-OS gcc kbuild test..."
	@rm -rf /tmp/icsos-gkbuild-iso
	@mkdir -p /tmp/icsos-gkbuild-iso/boot/grub /tmp/icsos-gkbuild-iso/apps/ldscripts
	@cp kernel/Kernel64.bin /tmp/icsos-gkbuild-iso/vmdex
	@cp /tmp/icsos-gcc/cc1 /tmp/icsos-gkbuild-iso/apps/cc1.exe
	@cp contrib/gccdriver/gcc.exe /tmp/icsos-gkbuild-iso/apps/gcc.exe
	@cp apps/as.exe /tmp/icsos-gkbuild-iso/apps/as.exe
	@cp apps/ld.exe /tmp/icsos-gkbuild-iso/apps/ld.exe
	@cp apps/ldscripts/elf_x86_64.x* /tmp/icsos-gkbuild-iso/apps/ldscripts/
	@cp apps/make.exe /tmp/icsos-gkbuild-iso/apps/make.exe
	@cp apps/objcopy.exe apps/cp.exe /tmp/icsos-gkbuild-iso/apps/
	@cp apps/hello.exe /tmp/icsos-gkbuild-iso/apps/hello.exe
	@cp /tmp/icsos-gcc/rt/*.o /tmp/icsos-gkbuild-iso/apps/
	@printf 'extern int puts(const char *s);\nint main(void)\n{\n   puts("GCC_E2E_OK");\n   return 0;\n}\n' \
		> /tmp/icsos-gkbuild-iso/apps/gccprobe.c
	@cp /tmp/icsos-gkbuild/ksrc.tar /tmp/icsos-gkbuild-iso/ksrc.tar
	@printf '%s\n' 'set PATH=/icsos/apps' 'kbuild' 'reboot' > /tmp/icsos-gkbuild-iso/autoexec.bat
	@printf '%s\n' 'set PATH=/icsos/apps' 'exectest' 'gctest' 'kexeccert' 'reboot' \
		> /tmp/icsos-gkbuild-iso/postkexec.bat
	@printf '%s\n' 'set timeout=0' 'menuentry "ics" { multiboot2 /vmdex selfhost-stage1; boot }' \
		> /tmp/icsos-gkbuild-iso/boot/grub/grub.cfg
	@grub-mkrescue -o /tmp/icsos-gkbuild.iso /tmp/icsos-gkbuild-iso >/dev/null 2>&1
	@echo "Formatting 512 MiB FAT16 kernel-build work disk..."
	@truncate -s 512M /tmp/icsos-gkbuild-work.img
	@mkfs.vfat -F 16 -s 32 /tmp/icsos-gkbuild-work.img >/dev/null
	@echo "Running in-OS gcc kbuild test (gcc -> cc1/as, ld, kexec) (KVM)..."
	@rm -f /tmp/icsos-gkbuild.log
	@-timeout 2400 $(QEMU_X64) -enable-kvm -cpu host -smp 2 -display none -monitor none \
		-serial file:/tmp/icsos-gkbuild.log -no-reboot -m 512M \
		-cdrom /tmp/icsos-gkbuild.iso -boot d \
		-drive file=/tmp/icsos-gkbuild-work.img,if=none,id=vd0,format=raw \
		-device virtio-blk-pci,drive=vd0,disable-legacy=on < /dev/null
	@grep -aq "Root mount \[OK\]" /tmp/icsos-gkbuild.log
	@grep -aq "work: mounted" /tmp/icsos-gkbuild.log
	@grep -aq "gmake-kbuild:" /tmp/icsos-gkbuild.log
	@grep -aq "GKBUILD_ORCHESTRATOR GNU_MAKE_3_82" /tmp/icsos-gkbuild.log
	@grep -aq "GKBUILD_LINK_OK" /tmp/icsos-gkbuild.log
	@grep -aq "GKBUILD_TEST_PASS" /tmp/icsos-gkbuild.log
	@grep -aq "KEXEC_BOOT_OK" /tmp/icsos-gkbuild.log
	@grep -aq "SMP: 2 CPUs online" /tmp/icsos-gkbuild.log
	@grep -aq "SMP: AP work-steal OK" /tmp/icsos-gkbuild.log
	@(grep -aq "EXEC_TEST_PASS" /tmp/icsos-gkbuild.log || \
	  (grep -aq "Hello World from ICS-OS!" /tmp/icsos-gkbuild.log && \
	   ! grep -aq "EXEC_TEST_FAIL" /tmp/icsos-gkbuild.log))
	@grep -aq "GCC_E2E_OK" /tmp/icsos-gkbuild.log
	@grep -aq "GCC_E2E_RUN_OK" /tmp/icsos-gkbuild.log
	@grep -aq "KEXEC_SMP_OK cpus=2" /tmp/icsos-gkbuild.log
	@grep -aq "KEXEC_CAPABILITY_PASS" /tmp/icsos-gkbuild.log
	@! grep -aq "GKBUILD_TEST_FAIL\|GCC_DRV_FAIL" /tmp/icsos-gkbuild.log
	@! grep -aq "KEXEC_CAPABILITY_FAIL\|EXEC_TEST_FAIL\|GCC_E2E_FAIL" /tmp/icsos-gkbuild.log
	@! grep -aq "POOL EMPTY\|userpd map failed" /tmp/icsos-gkbuild.log
	@echo "test-gcc-kbuild PASS"

# Canonical kernel self-host test: GCC compiles the complete kernel in ICS-OS.
test-kbuild: test-gcc-kbuild
	@echo "test-kbuild PASS (GCC)"

# Strict self-host certification. Unlike test-kbuild, this does not accept the
# host-seeded cc1 as the final compiler: GCC-owned frontend/support objects and
# the SDK runtime are rebuilt in ICS-OS, the rebuilt cc1 builds a new gcc
# driver, and that in-OS-built toolchain rebuilds + kexecs the kernel. The new
# kernel must then pass SMP, ELF exec, and GCC/binutils capability checks.
test-selfhost-cert: vmdex apps/as.exe apps/ld.exe apps/ar.exe apps/hello.exe apps/make.exe apps/objcopy.exe apps/cp.exe apps/mkdir.exe apps/rm.exe
	@echo "Updating stage-1 cc1 seed (contrib/gcc)..."
	@$(MAKE) -C contrib/gcc cc1
	@$(MAKE) -C contrib/gccdriver
	@$(MAKE) -C contrib/gmp all
	@$(MAKE) -C contrib/mpfr all
	@$(MAKE) -C contrib/mpc all
	@chmod +x scripts/stage-gcc-selfhost.sh scripts/stage-kernel-src.sh scripts/stage-make-gcc.sh
	@scripts/stage-gcc-selfhost.sh /tmp/icsos-gccself
	@scripts/stage-kernel-src.sh /tmp/icsos-gccself-kernel
	@scripts/stage-make-gcc.sh /tmp/icsos-gccself-make
	@tar --format=ustar -C /tmp/icsos-gccself-make -cf /tmp/icsos-gccself-makesrc.tar .
	@mkdir -p /tmp/icsos-gcc/rt
	@for src in crt1 tccsdk libtcc1 posix setjmp; do \
	   gcc -m64 -std=gnu89 -w -nostdlib -fno-builtin -static -ffreestanding \
	      -fno-pie -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables \
	      -fno-strict-aliasing -mcmodel=large -mno-red-zone -Isdk/include \
	      -c sdk/$$src.c -o /tmp/icsos-gcc/rt/$$src.o || exit 1; \
	done
	@rm -rf /tmp/icsos-gccself-iso
	@mkdir -p /tmp/icsos-gccself-iso/boot/grub \
		/tmp/icsos-gccself-iso/apps/ldscripts
	@cp kernel/Kernel64.bin /tmp/icsos-gccself-iso/vmdex
	@cp /tmp/icsos-gcc/cc1 /tmp/icsos-gccself-iso/apps/cc1.exe
	@cp contrib/gccdriver/gcc.exe /tmp/icsos-gccself-iso/apps/gcc.exe
	@cp apps/as.exe apps/ar.exe apps/ld.exe apps/hello.exe \
		/tmp/icsos-gccself-iso/apps/
	@cp apps/make.exe /tmp/icsos-gccself-iso/apps/make.exe
	@cp apps/objcopy.exe apps/cp.exe apps/mkdir.exe apps/rm.exe \
		/tmp/icsos-gccself-iso/apps/
	@cp apps/ldscripts/elf_x86_64.x* /tmp/icsos-gccself-iso/apps/ldscripts/
	@cp /tmp/icsos-gcc/rt/*.o /tmp/icsos-gccself-iso/apps/
	@cp -a /tmp/icsos-gccself/gccsrc /tmp/icsos-gccself/seed \
		/tmp/icsos-gccself-iso/
	@cp /tmp/icsos-gccself-kernel/ksrc.tar /tmp/icsos-gccself-iso/ksrc.tar
	@cp /tmp/icsos-gccself-makesrc.tar /tmp/icsos-gccself-iso/makesrc.tar
	@printf 'extern int puts(const char *s);\nint main(void)\n{\n   puts("GCC_E2E_OK");\n   return 0;\n}\n' \
		> /tmp/icsos-gccself-iso/apps/gccprobe.c
	@printf '%s\n' 'set PATH=/icsos/apps' 'gccselfhost' 'reboot' \
		> /tmp/icsos-gccself-iso/autoexec.bat
	@printf '%s\n' 'set PATH=/icsos/apps' 'exectest' 'gccclosed' 'kexeccert' 'reboot' \
		> /tmp/icsos-gccself-iso/postkexec.bat
	@printf '%s\n' 'set timeout=0' 'menuentry "ics" { multiboot2 /vmdex selfhost-stage1; boot }' \
		> /tmp/icsos-gccself-iso/boot/grub/grub.cfg
	@grub-mkrescue -o /tmp/icsos-gccself.iso /tmp/icsos-gccself-iso >/dev/null 2>&1
	@truncate -s 512M /tmp/icsos-gccself-work.img
	@mkfs.vfat -F 16 -s 32 /tmp/icsos-gccself-work.img >/dev/null
	@echo "Running strict GCC self-host closure test (may take several hours)..."
	@rm -f /tmp/icsos-gccself.log
	@-timeout 28800 $(QEMU_X64) -enable-kvm -cpu host -smp 1 \
		-display none -monitor none -serial file:/tmp/icsos-gccself.log \
		-no-reboot -m 2048M -cdrom /tmp/icsos-gccself.iso -boot d \
		-drive file=/tmp/icsos-gccself-work.img,if=none,id=vd0,format=raw \
		-device virtio-blk-pci,drive=vd0,disable-legacy=on \
		< /dev/null
	@grep -aq "GCC_SELF_OBJECTS_OK 349" /tmp/icsos-gccself.log
	@grep -aq "GCC_SELF_ORCHESTRATOR GNU_MAKE_3_82" /tmp/icsos-gccself.log
	@grep -aq "GCC_SELF_REBUILD_OK" /tmp/icsos-gccself.log
	@grep -aq "GCC_SELF_LOOP_OK" /tmp/icsos-gccself.log
	@grep -aq "GCC_SELF_PASS" /tmp/icsos-gccself.log
	@grep -aq "GCC_SELF_CERT_COMPILER_OK" /tmp/icsos-gccself.log
	@grep -aq "GMAKE_SELF_ORCHESTRATOR GNU_MAKE_3_82" /tmp/icsos-gccself.log
	@grep -aq "GMAKE_SELF_REBUILT_WITH_GCC" /tmp/icsos-gccself.log
	@grep -aq "GMAKE_SELF_TEST_PASS" /tmp/icsos-gccself.log
	@grep -aq "GKBUILD_COMPILER_PROVENANCE in-os-rebuilt" /tmp/icsos-gccself.log
	@grep -aq "GKBUILD_ORCHESTRATOR GNU_MAKE_3_82" /tmp/icsos-gccself.log
	@grep -aq "GKBUILD_TEST_PASS" /tmp/icsos-gccself.log
	@grep -aq "KEXEC_BOOT_OK" /tmp/icsos-gccself.log
	@(grep -aq "EXEC_TEST_PASS" /tmp/icsos-gccself.log || \
	  (grep -aq "Hello World from ICS-OS!" /tmp/icsos-gccself.log && \
	   ! grep -aq "EXEC_TEST_FAIL" /tmp/icsos-gccself.log))
	@grep -aq "GCC_E2E_OK" /tmp/icsos-gccself.log
	@grep -aq "GCC_CLOSED_TOOLCHAIN_RUN_OK" /tmp/icsos-gccself.log
	@grep -aq "KEXEC_CAPABILITY_PASS" /tmp/icsos-gccself.log
	@! grep -aq "GCC_SELF_.*FAIL\|GMAKE_SELF_FAIL\|GCC_DRV_FAIL\|GCC_CLOSED_TOOLCHAIN_FAIL\|GKBUILD_TEST_FAIL\|KEXEC_CAPABILITY_FAIL\|GCC_E2E_FAIL\|POOL EMPTY\|userpd map failed" /tmp/icsos-gccself.log
	@echo "test-selfhost-cert PASS"

# Full QEMU integration suite for the x86_64 / SMP / ELF64 path.
test-integration: vmdex apps/hello.exe
	@echo "=== ICS-OS integration tests ==="
	$(MAKE) test-boot
	$(MAKE) test-smp
	$(MAKE) test-exec
	@echo "=== test-integration PASS ==="
