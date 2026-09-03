#!/bin/bash
# Validate in-kernel GPT detection, per-partition registration, and durable
# partition I/O through QEMU IDE (-hda).
#
# The GPT disk (ics-os-gpt.img, built by scripts/mkgpt.sh) is attached as the
# primary IDE drive. The system still boots from the CD-ROM (root on cdfs),
# so this is a Phase-1 gate: GPT_DETECT + PART_REG + FAT partition I/O.
# The guest mounts the FAT32 partition (hdp0p0) at /gpt, copies a file onto it
# (cp does fsync -> block-cache flush -> IDE write), and the host reads the
# file back off the partition and compares it byte-for-byte.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

SOURCE_IMAGE="${1:-ics-os-gpt.img}"
DISK_GUID="${ICSOS_GPT_DISK_GUID:-6a8b2c3d-4e5f-4a6b-8c7d-9e0f1a2b3c4d}"
P0_START=2048            # FAT32 partition slot 0 start LBA (must match mkgpt.sh)
TIMEOUT_SECONDS="${QEMU_GPT_TIMEOUT_SECONDS:-90}"
QEMU_BIN="${QEMU_X64:-qemu-system-x86_64}"
QEMU_SMP="${QEMU_SMP:-1}"
RUN_ID="$(date -u +%Y%m%dT%H%M%SZ)-$$"
ARTIFACT_DIR="${QEMU_GPT_ARTIFACT_DIR:-/tmp/icsos-tests/gpt-$RUN_ID}"
WORK_DIR="$ARTIFACT_DIR/work"
RAW_IMAGE="$WORK_DIR/ics-os-gpt.img"
ISO_ROOT="$WORK_DIR/iso-root"
BOOT_ISO="$WORK_DIR/boot.iso"
SERIAL_LOG="$ARTIFACT_DIR/serial.log"
PAYLOAD_SOURCE="$WORK_DIR/gpt-source.txt"
PAYLOAD_RESULT="$ARTIFACT_DIR/gpt-result.txt"
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

for tool in "$QEMU_BIN" grub-mkrescue mcopy mdel; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "test-qemu-gpt: missing required tool: $tool" >&2
        exit 1
    fi
done
if [ ! -f "$SOURCE_IMAGE" ]; then
    echo "test-qemu-gpt: image not found: $SOURCE_IMAGE (run scripts/mkgpt.sh)" >&2
    exit 1
fi
if [ ! -f kernel/Kernel64.bin ]; then
    echo "test-qemu-gpt: kernel/Kernel64.bin is missing; run make vmdex" >&2
    exit 1
fi
if [ ! -f apps/cp.exe ]; then
    echo "test-qemu-gpt: apps/cp.exe is missing; run make apps" >&2
    exit 1
fi

mkdir -p "$WORK_DIR" "$ISO_ROOT/boot/grub" "$ISO_ROOT/apps"
touch "$SERIAL_LOG"
cp "$SOURCE_IMAGE" "$RAW_IMAGE"
cp kernel/Kernel64.bin "$ISO_ROOT/vmdex"
cp apps/cp.exe "$ISO_ROOT/apps/cp.exe"
printf '%s\n' 'set timeout=0' \
    "menuentry \"ics\" { multiboot2 /vmdex; boot }" \
    > "$ISO_ROOT/boot/grub/grub.cfg"
printf '%s\n' \
    '@echo off' \
    'set PATH=/icsos/apps' \
    'mount fat hdp0p0 gpt' \
    'cp.exe -v /gpt/GPTSRC.TXT /gpt/GPTRES.TXT' \
    > "$AUTOEXEC"
cp "$AUTOEXEC" "$ISO_ROOT/autoexec.bat"
grub-mkrescue -o "$BOOT_ISO" "$ISO_ROOT" >/dev/null 2>&1

# Place the source payload at the root of the FAT32 partition (slot 0).
printf 'ics-os QEMU GPT durable partition write\n' > "$PAYLOAD_SOURCE"
P0_OFF=$((P0_START * 512))
mcopy -o -i "${RAW_IMAGE}@@${P0_OFF}" "$PAYLOAD_SOURCE" ::GPTSRC.TXT
mdel -i "${RAW_IMAGE}@@${P0_OFF}" ::GPTRES.TXT >/dev/null 2>&1 || true

"$QEMU_BIN" -smp "$QEMU_SMP" -nographic -no-reboot -m 128M \
    -hda "$RAW_IMAGE" \
    -cdrom "$BOOT_ISO" -boot d \
    < /dev/null > "$SERIAL_LOG" 2>&1 &
QEMU_PID=$!

if ! timeout "$TIMEOUT_SECONDS" grep -a -m1 -q \
    'cp: copied /gpt/GPTSRC.TXT -> /gpt/GPTRES.TXT' \
    < <(tail -n +1 -F "$SERIAL_LOG" 2>/dev/null); then
    echo "test-qemu-gpt: guest marker timed out" >&2
    tail -n 120 "$SERIAL_LOG" >&2 || true
    exit 1
fi
kill "$QEMU_PID" 2>/dev/null || true
wait "$QEMU_PID" 2>/dev/null || true
QEMU_PID=""

# Host readback: read the guest-written file off the FAT32 partition and compare.
mcopy -i "${RAW_IMAGE}@@${P0_OFF}" ::GPTRES.TXT "$PAYLOAD_RESULT"
cmp "$PAYLOAD_SOURCE" "$PAYLOAD_RESULT"

# Serial assertions: detection, per-partition registration, root from CD,
# in-guest mount, durable copy, and absence of GPT warnings / faults.
grep -a -q "GPT_DETECT hdp0 entries=2 diskguid=${DISK_GUID}" "$SERIAL_LOG"
grep -a -q 'PART_REG hdp0 hdp0p0 start=2048 end=67584 type=Basic Data name=icsos-fat' "$SERIAL_LOG"
grep -a -q 'PART_REG hdp0 hdp0p1 start=67584 end=100352 type=Linux data name=icsos-ext4' "$SERIAL_LOG"
grep -a -q 'Root filesystem is the CD-ROM (cdfs).' "$SERIAL_LOG"
grep -a -q 'Root mount \[OK\]' "$SERIAL_LOG"
grep -a -q 'mount successful.' "$SERIAL_LOG"
grep -a -q 'cp: copied /gpt/GPTSRC.TXT -> /gpt/GPTRES.TXT' "$SERIAL_LOG"
! grep -a -q 'GPT_WARN' "$SERIAL_LOG"
! grep -a -q 'General Protection fault\|Page fault\|Double fault\|Divide by zero' "$SERIAL_LOG"
! grep -a -q 'cp: failed' "$SERIAL_LOG"

echo "test-qemu-gpt PASS"
echo "Artifacts: $ARTIFACT_DIR"
