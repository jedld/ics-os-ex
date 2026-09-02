#!/bin/bash
# Document the expected xHCI gap without treating CD fallback as USB success.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

SOURCE_IMAGE="${1:-ics-os-usb.img}"
TIMEOUT_SECONDS="${QEMU_XHCI_TIMEOUT_SECONDS:-60}"
QEMU_BIN="${QEMU_X64:-qemu-system-x86_64}"
RUN_ID="$(date -u +%Y%m%dT%H%M%SZ)-$$"
ARTIFACT_DIR="${QEMU_XHCI_ARTIFACT_DIR:-/tmp/icsos-tests/xhci-gap-$RUN_ID}"
WORK_DIR="$ARTIFACT_DIR/work"
RAW_IMAGE="$WORK_DIR/ics-os-usb.img"
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
        echo "test-qemu-xhci-gap: missing required tool: $tool" >&2
        exit 1
    fi
done
if [ ! -f "$SOURCE_IMAGE" ] || [ ! -f kernel/Kernel64.bin ]; then
    echo "test-qemu-xhci-gap: USB image or kernel is missing" >&2
    exit 1
fi

mkdir -p "$WORK_DIR" "$ISO_ROOT/boot/grub"
touch "$SERIAL_LOG"
cp "$SOURCE_IMAGE" "$RAW_IMAGE"
cp kernel/Kernel64.bin "$ISO_ROOT/vmdex"
printf '%s\n' 'set timeout=0' \
    'menuentry "ics" { multiboot2 /vmdex; boot }' \
    > "$ISO_ROOT/boot/grub/grub.cfg"
grub-mkrescue -o "$BOOT_ISO" "$ISO_ROOT" >/dev/null 2>&1

"$QEMU_BIN" -nographic -no-reboot -m 128M \
    -cdrom "$BOOT_ISO" -boot d \
    -drive if=none,id=stick,format=raw,file="$RAW_IMAGE" \
    -device qemu-xhci,id=xhci \
    -device usb-storage,bus=xhci.0,drive=stick \
    < /dev/null > "$SERIAL_LOG" 2>&1 &
QEMU_PID=$!

if ! timeout "$TIMEOUT_SECONDS" grep -a -m1 -q \
    'Running console thread' < <(tail -n +1 -F "$SERIAL_LOG" 2>/dev/null); then
    echo "test-qemu-xhci-gap: guest startup timed out" >&2
    tail -n 100 "$SERIAL_LOG" >&2 || true
    exit 1
fi
kill "$QEMU_PID" 2>/dev/null || true
wait "$QEMU_PID" 2>/dev/null || true
QEMU_PID=""

grep -a -q 'boot_device_name=cds0' "$SERIAL_LOG"
grep -a -q 'usb: no UHCI controller found' "$SERIAL_LOG"
grep -a -q 'Root filesystem is the CD-ROM (cdfs).' "$SERIAL_LOG"
grep -a -q 'Root mount \[OK\]' "$SERIAL_LOG"
! grep -a -q 'usb: registered block device usb0\|Root filesystem is the USB mass-storage device.' "$SERIAL_LOG"
! grep -a -q 'General Protection fault\|Page fault\|Double fault\|Divide by zero' "$SERIAL_LOG"

echo "test-qemu-xhci-gap EXPECTED_GAP xHCI unsupported"
echo "Artifacts: $ARTIFACT_DIR"