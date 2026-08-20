# ics-os Makefile
#
# written by Joseph Anthony C. Hermocilla
#

QEMU_I386=qemu-system-i386
QEMU_X64=qemu-system-x86_64
QEMU_MEM=64M
QEMU_NET=-net nic,model=rtl8139 -net none
# QEMU can load the 32-bit multiboot kernel the same way GRUB does.
QEMU_KERNEL=-kernel kernel/Kernel32.bin

vmdex:
	make -C kernel/

.PHONY: clean usb floppy livecd apps
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
	if [ -d sdk ] && ls sdk/* >/dev/null 2>&1; then cp sdk/* tmp/tcc1/; fi
	if [ -d lib ] && ls lib/* >/dev/null 2>&1; then cp lib/* tmp/lib1/; fi

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
usb: prep_image
	chmod +x scripts/mkusb.sh
	scripts/mkusb.sh
	rm -fr tmp/ tmp-usb

apps:
	$(MAKE) -C contrib/hello
	$(MAKE) -C contrib/hello install || true

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
boot-usb:
	$(QEMU_I386) $(QEMU_NET) -m $(QEMU_MEM) $(QEMU_KERNEL) \
		-drive file=ics-os-usb.img,format=raw,if=ide -boot c

# Same image on a 64-bit CPU (protected-mode kernel on AMD64).
boot-usb-amd64:
	$(QEMU_X64) $(QEMU_NET) -m $(QEMU_MEM) $(QEMU_KERNEL) \
		-drive file=ics-os-usb.img,format=raw,if=ide -boot c

# Boot from a real UHCI USB mass-storage device and mount it as root.
boot-usb-storage:
	$(QEMU_X64) $(QEMU_NET) -m $(QEMU_MEM) $(QEMU_KERNEL) \
		-drive if=none,id=stick,format=raw,file=ics-os-usb.img \
		-device piix3-usb-uhci,id=uhci \
		-device usb-storage,bus=uhci.0,drive=stick

boot-usb-uefi:
	$(QEMU_X64) $(QEMU_NET) -m $(QEMU_MEM) -bios /usr/share/ovmf/OVMF.fd \
		-drive file=ics-os-livecd.iso,format=raw,if=ide -boot c

# Headless emulator smoke tests (serial console, no VGA window).
test-usb:
	timeout 25 $(QEMU_I386) -display none -serial stdio -no-reboot -m $(QEMU_MEM) \
		$(QEMU_KERNEL) -drive file=ics-os-usb.img,format=raw,if=ide -boot c

test-usb-amd64:
	timeout 25 $(QEMU_X64) -display none -serial stdio -no-reboot -m $(QEMU_MEM) \
		$(QEMU_KERNEL) -drive file=ics-os-usb.img,format=raw,if=ide -boot c

test-usb-storage:
	timeout 40 $(QEMU_X64) -display none -serial stdio -no-reboot -m $(QEMU_MEM) \
		$(QEMU_KERNEL) \
		-drive if=none,id=stick,format=raw,file=ics-os-usb.img \
		-device piix3-usb-uhci,id=uhci \
		-device usb-storage,bus=uhci.0,drive=stick
