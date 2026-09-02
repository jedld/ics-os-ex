# Intel N150 USB boot and working-storage readiness

## Goal

Boot ICS-OS on an Intel N150 laptop from a USB thumb drive and keep the same
FAT32 partition mounted at `/icsos` as writable working storage.

## Current qualification

The generated `ics-os-usb.img` is a DOS/MBR disk with one bootable FAT32
partition starting at LBA 2048. It contains GRUB for legacy BIOS and
`EFI/BOOT/BOOTX64.EFI` for 64-bit UEFI. Both paths load the ELF64 Multiboot2
kernel.

The following VirtualBox 7.1 checks pass with the image attached as a PIIX4 IDE
disk:

- BIOS GRUB boot, `hdp0p0` FAT32 root mount, file creation, `fsync`, poweroff,
  raw-image extraction, and byte-for-byte host comparison;
- UEFI GRUB boot and the same persistent-write/readback sequence;
- cleanup uses a disposable image and VM and never opens a physical drive.

Run these gates before flashing:

```bash
cd ics-os
make usb
make test-usb-storage
make test-usb-storage-xhci-gap
make test-vbox-usb-image
make test-vbox-usb-image-efi
```

The QEMU UHCI gate boots the kernel from a separate CD, attaches a disposable
copy of the image only through `piix3-usb-uhci`, requires `usb0p0` to become the
root, performs a guest write and `fsync`, and compares the persisted bytes on
the host. The xHCI expected-gap lane attaches the same image through
`qemu-xhci` and requires the kernel to leave it undiscovered while using the CD
root. A CD fallback can therefore never be mistaken for USB success.

VirtualBox IDE validates firmware discovery, GRUB, kernel boot, FAT32 mounting,
allocation, block-cache writeback, and persistence. It does not validate that
the kernel can operate the laptop's physical USB controller.

## N150 blocker

Intel N150-class laptops expose modern USB ports through an xHCI controller.
ICS-OS currently has only a polling UHCI host controller and USB mass-storage
path. GRUB firmware can read the stick and load the kernel, but after firmware
services end the kernel must own xHCI to continue reading `/icsos`. Without an
xHCI driver, physical boot is expected to fail at root-device discovery or root
mount unless the firmware provides a nonstandard legacy handoff.

xHCI support is therefore the release blocker for using the same physical stick
as the root and working disk. Required work includes:

1. PCI xHCI discovery, BAR mapping with uncacheable MMIO attributes, BIOS/OS
   ownership handoff, reset, halt, and run-state management.
2. DMA-safe command, event, and transfer rings; 64-bit DMA addressing; cycle-bit
   handling; memory barriers; and cache coherency through a shared DMA API.
3. Root-hub port reset and state changes, device-slot/address/configuration
   commands, endpoint contexts, and control transfers.
4. Bulk-only transport for USB mass storage over xHCI, including stalls,
   timeouts, reset recovery, short transfers, media removal, and flush behavior.
5. Interrupt support (prefer MSI/MSI-X), polling fallback for bring-up, bounded
   waits, cancellation, teardown, and surprise-removal safety.
6. Stable topology and transfer diagnostics plus QEMU xHCI and physical-device
   qualification tests.

QEMU now provides a passing UHCI qualification lane and an xHCI expected-gap
lane. The latter should become a persistence PASS gate as xHCI is implemented.
Emulation is useful for driver development but does not replace testing the
exact laptop controller, firmware, ports, and thumb drive. VirtualBox does not
provide a useful UHCI compatibility lane for this image.

## Other laptop gaps

These do not all block initial serial/VGA boot, but they block a practical or
production-grade N150 laptop experience:

| Area | Current gap | Minimum qualification |
|---|---|---|
| Firmware | UEFI works in VirtualBox; Secure Boot is unsupported | Disable Secure Boot initially; later sign a measured boot chain and define key/update policy |
| ACPI | Modern topology, interrupt routing, power, battery, lid, and sleep support is incomplete | Parse required ACPI tables, validate APIC routing, orderly poweroff, thermal and battery reporting |
| Graphics | Legacy VGA is the current display path | GOP framebuffer handoff or native Intel graphics modesetting; resolution and console tests |
| Input | Legacy keyboard/mouse assumptions | xHCI HID keyboard/touchpad support or laptop-specific PS/2 validation |
| Internal storage | IDE and virtio paths do not cover typical NVMe hardware | NVMe queues, DMA, MSI-X, flush/FUA, timeout/reset, and power-loss tests |
| Networking | RTL8139 does not match typical N150 laptop Ethernet/Wi-Fi | Driver for the exact PCI/USB NIC; Wi-Fi also needs firmware, regulatory, authentication, and crypto support |
| Audio | No modern laptop audio stack | PCI/HDA or SoundWire support for the exact hardware |
| Reliability | FAT has no journal and current recovery coverage is limited | Clean shutdown, durable metadata ordering, corruption detection/repair, removal and power-loss testing |
| Security | No production Secure Boot/IOMMU posture | IOMMU-backed DMA isolation, least-privilege drivers, signed updates, and threat-model validation |

## Capacity and flashing

The default image is intentionally 128 MiB for fast emulator testing. It will
not automatically occupy a 16 GB drive. Build a larger image below the drive's
actual byte capacity, leaving margin for vendor size differences. For example:

```bash
cd ics-os
ICSOS_USB_SIZE_MB=14000 make usb
```

Do not select a size larger than the target device. Check the device immediately
before flashing and ensure none of its partitions are mounted:

```bash
lsblk -o NAME,PATH,SIZE,MODEL,SERIAL,TRAN,MOUNTPOINTS
sudo umount /dev/sdX1
sudo dd if=ics-os-usb.img of=/dev/sdX bs=4M status=progress conv=fsync
sync
```

Replace `/dev/sdX` with the whole thumb drive, never a partition. All data on
that drive is destroyed. Do not flash while the VirtualBox gates fail.

## Physical acceptance gate

The image is not N150-ready until testing on the target laptop proves all of the
following through serial or persistent structured logs:

- Secure Boot state and UEFI boot path are recorded;
- xHCI ownership transfer succeeds and the boot stick is enumerated;
- `/icsos` mounts from USB, not an emulator-only IDE fallback;
- create, overwrite, `fsync`, reboot, and byte-for-byte readback pass;
- repeated multi-megabyte I/O passes without timeout, corruption, or DMA fault;
- unplug during idle and active I/O fails safely without use-after-free;
- controller reset and media reattach recover predictably;
- SMP contention and sustained I/O complete without faults;
- clean shutdown flushes data, and injected power loss has a documented recovery
  result.

Until xHCI and this hardware gate pass, retain the image as an emulator-qualified
BIOS/UEFI disk image, not as a production-ready N150 USB installation.
