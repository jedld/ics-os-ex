#!/bin/bash
# Validate durable FAT-root writes through QEMU UHCI USB mass storage.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

SOURCE_IMAGE="${1:-ics-os-usb.img}"
TIMEOUT_SECONDS="${QEMU_UHCI_TIMEOUT_SECONDS:-90}"
QEMU_BIN="${QEMU_X64:-qemu-system-x86_64}"
RUN_ID="$(date -u +%Y%m%dT%H%M%SZ)-$$"
ARTIFACT_DIR="${QEMU_UHCI_ARTIFACT_DIR:-/tmp/icsos-tests/uhci-usb-$RUN_ID}"
WORK_DIR="$ARTIFACT_DIR/work"
RAW_IMAGE="$WORK_DIR/ics-os-usb.img"
ISO_ROOT="$WORK_DIR/iso-root"
BOOT_ISO="$WORK_DIR/boot.iso"
SERIAL_LOG="$ARTIFACT_DIR/serial.log"
PAYLOAD_SOURCE="$WORK_DIR/persist-source.txt"
PAYLOAD_RESULT="$ARTIFACT_DIR/persist-result.txt"
AUTOEXEC="$WORK_DIR/autoexec.bat"
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

for tool in "$QEMU_BIN" grub-mkrescue mcopy mmd mdir sfdisk python3; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "test-qemu-uhci-storage: missing required tool: $tool" >&2
        exit 1
    fi
done
if [ ! -f "$SOURCE_IMAGE" ]; then
    echo "test-qemu-uhci-storage: image not found: $SOURCE_IMAGE" >&2
    exit 1
fi
if [ ! -f kernel/Kernel64.bin ]; then
    echo "test-qemu-uhci-storage: kernel/Kernel64.bin is missing; run make vmdex" >&2
    exit 1
fi
if [ ! -f apps/cp.exe ]; then
    echo "test-qemu-uhci-storage: apps/cp.exe is missing; run make apps" >&2
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

printf 'ics-os QEMU UHCI persistent root write\n' > "$PAYLOAD_SOURCE"
cat > "$AUTOEXEC" <<'EOF'
@echo off
set PATH=/icsos/apps
cp-posix.exe /icsos/work/UHCISRC.TXT /icsos/work/UHCIRES.TXT
echo UHCIOK
echo UHCIOK
echo UHCIOK
EOF

PART_START="$(sfdisk -J "$RAW_IMAGE" | python3 -c \
    'import json,sys; print(json.load(sys.stdin)["partitiontable"]["partitions"][0]["start"])')"
OFFSET=$((PART_START * 512))
if ! mdir -i "${RAW_IMAGE}@@${OFFSET}" ::work >/dev/null 2>&1; then
    mmd -i "${RAW_IMAGE}@@${OFFSET}" ::work
fi
mcopy -o -i "${RAW_IMAGE}@@${OFFSET}" "$PAYLOAD_SOURCE" ::work/UHCISRC.TXT
mcopy -o -i "${RAW_IMAGE}@@${OFFSET}" "$AUTOEXEC" ::autoexec.bat
mcopy -o -i "${RAW_IMAGE}@@${OFFSET}" apps/cp.exe ::apps/cp-posix.exe

"$QEMU_BIN" -nographic -no-reboot -m 128M \
    -cdrom "$BOOT_ISO" -boot d \
    -drive if=none,id=stick,format=raw,file="$RAW_IMAGE" \
    -device piix3-usb-uhci,id=uhci \
    -device usb-storage,bus=uhci.0,drive=stick \
    < /dev/null > "$SERIAL_LOG" 2>&1 &
QEMU_PID=$!

if ! timeout "$TIMEOUT_SECONDS" grep -a -m1 -q \
    'UHCIOK' < <(tail -n +1 -F "$SERIAL_LOG" 2>/dev/null); then
    echo "test-qemu-uhci-storage: guest marker timed out" >&2
    tail -n 100 "$SERIAL_LOG" >&2 || true
    exit 1
fi
kill "$QEMU_PID" 2>/dev/null || true
wait "$QEMU_PID" 2>/dev/null || true
QEMU_PID=""

mcopy -i "${RAW_IMAGE}@@${OFFSET}" ::work/UHCIRES.TXT "$PAYLOAD_RESULT"
cmp "$PAYLOAD_SOURCE" "$PAYLOAD_RESULT"
grep -a -q 'boot_device_name=cds0' "$SERIAL_LOG"
grep -a -q 'usb: UHCI at PCI' "$SERIAL_LOG"
grep -a -q 'usb: registered usb0p0' "$SERIAL_LOG"
grep -a -q 'Root filesystem is the USB mass-storage device.' "$SERIAL_LOG"
grep -a -q 'Root mount \[OK\]' "$SERIAL_LOG"
! grep -a -q 'General Protection fault\|Page fault\|Double fault\|Divide by zero' "$SERIAL_LOG"

echo "test-qemu-uhci-storage PASS"
echo "Artifacts: $ARTIFACT_DIR"