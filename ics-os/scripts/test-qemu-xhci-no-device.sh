#!/bin/bash
# Verify clean xHCI initialization and fallback when no USB device is attached.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

TIMEOUT_SECONDS="${QEMU_XHCI_TIMEOUT_SECONDS:-60}"
QEMU_BIN="${QEMU_X64:-qemu-system-x86_64}"
QEMU_MACHINE="${QEMU_MACHINE:-q35}"
RUN_ID="$(date -u +%Y%m%dT%H%M%SZ)-$$"
ARTIFACT_DIR="${QEMU_XHCI_ARTIFACT_DIR:-/tmp/icsos-tests/xhci-no-device-$RUN_ID}"
WORK_DIR="$ARTIFACT_DIR/work"
ISO_ROOT="$WORK_DIR/iso-root"
BOOT_ISO="$WORK_DIR/boot.iso"
SERIAL_LOG="$ARTIFACT_DIR/serial.log"
QEMU_PID=""

cleanup()
{
    if [ -n "$QEMU_PID" ] && kill -0 "$QEMU_PID" 2>/dev/null; then
        kill "$QEMU_PID" 2>/dev/null || true
        wait "$QEMU_PID" 2>/dev/null || true
    fi
    rm -rf "$WORK_DIR"
}
trap cleanup EXIT

for tool in "$QEMU_BIN" grub-mkrescue; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "test-qemu-xhci-no-device: missing required tool: $tool" >&2
        exit 1
    fi
done
if [ ! -f kernel/Kernel64.bin ]; then
    echo "test-qemu-xhci-no-device: kernel/Kernel64.bin is missing" >&2
    exit 1
fi

mkdir -p "$WORK_DIR" "$ISO_ROOT/boot/grub"
touch "$SERIAL_LOG"
cp kernel/Kernel64.bin "$ISO_ROOT/vmdex"
printf '%s\n' 'set timeout=0' \
    'menuentry "ics" { multiboot2 /vmdex; boot }' \
    > "$ISO_ROOT/boot/grub/grub.cfg"
grub-mkrescue -o "$BOOT_ISO" "$ISO_ROOT" >/dev/null 2>&1

"$QEMU_BIN" -machine "$QEMU_MACHINE" -nographic -no-reboot -m 128M \
    -cdrom "$BOOT_ISO" -boot d \
    -device qemu-xhci,id=xhci \
    < /dev/null > "$SERIAL_LOG" 2>&1 &
QEMU_PID=$!

if ! timeout "$TIMEOUT_SECONDS" grep -a -m1 -q \
    'Running console thread' < <(tail -n +1 -F "$SERIAL_LOG" 2>/dev/null); then
    echo "test-qemu-xhci-no-device: guest startup timed out" >&2
    tail -n 100 "$SERIAL_LOG" >&2 || true
    exit 1
fi
kill "$QEMU_PID" 2>/dev/null || true
wait "$QEMU_PID" 2>/dev/null || true
QEMU_PID=""

grep -a -q 'boot_device_name=cds0' "$SERIAL_LOG"
grep -a -q 'usb: no UHCI controller found; probing xHCI' "$SERIAL_LOG"
grep -a -q 'xhci: no enabled device port' "$SERIAL_LOG"
if [ "$QEMU_MACHINE" = q35 ]; then
    grep -a -q 'Warning: no root filesystem mounted (continuing).' "$SERIAL_LOG"
else
    grep -a -q 'Root filesystem is the CD-ROM (cdfs).' "$SERIAL_LOG"
    grep -a -q 'Root mount \[OK\]' "$SERIAL_LOG"
fi
! grep -a -q 'usb: registered block device usb0\|Root filesystem is the USB mass-storage device.' "$SERIAL_LOG"
! grep -a -q 'General Protection fault\|Page fault\|Double fault\|Divide by zero' "$SERIAL_LOG"

echo "test-qemu-xhci-no-device PASS"
echo "Artifacts: $ARTIFACT_DIR"