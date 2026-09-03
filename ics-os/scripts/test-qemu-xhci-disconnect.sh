#!/bin/bash
# Remove an active q35 xHCI storage device and require bounded I/O failure.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

SOURCE_IMAGE="${1:-ics-os-usb.img}"
QEMU_BIN="${QEMU_X64:-qemu-system-x86_64}"
TEST_MODE="${QEMU_XHCI_TEST_MODE:-disconnect}"
RUN_ID="$(date -u +%Y%m%dT%H%M%SZ)-$$"
ARTIFACT_DIR="${QEMU_XHCI_ARTIFACT_DIR:-/tmp/icsos-tests/xhci-$TEST_MODE-$RUN_ID}"
WORK_DIR="$ARTIFACT_DIR/work"
RAW_IMAGE="$WORK_DIR/ics-os-usb.img"
RECONNECT_IMAGE="$WORK_DIR/reconnect.img"
ISO_ROOT="$WORK_DIR/iso-root"
BOOT_ISO="$WORK_DIR/boot.iso"
SERIAL_LOG="$ARTIFACT_DIR/serial.log"
QMP_SOCKET="$WORK_DIR/qmp.sock"
QEMU_PID=""

if [ "$TEST_MODE" != disconnect ] && [ "$TEST_MODE" != mounted-disconnect ] &&
    [ "$TEST_MODE" != mounted-reconnect ] &&
    [ "$TEST_MODE" != mounted-remount ] &&
    [ "$TEST_MODE" != hotplug ] &&
    [ "$TEST_MODE" != hotplug-identity-mismatch ] &&
    [ "$TEST_MODE" != reconnect ] &&
    [ "$TEST_MODE" != reconnect-identity-mismatch ] &&
   [ "$TEST_MODE" != reconnect-mismatch ]; then
    echo "test-qemu-xhci-disconnect: invalid mode: $TEST_MODE" >&2
    exit 1
fi

cleanup()
{
    if [ -n "$QEMU_PID" ] && kill -0 "$QEMU_PID" 2>/dev/null; then
        kill "$QEMU_PID" 2>/dev/null || true
        wait "$QEMU_PID" 2>/dev/null || true
    fi
    rm -rf "$WORK_DIR"
}
trap cleanup EXIT

for tool in "$QEMU_BIN" grub-mkrescue python3 truncate; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "test-qemu-xhci-disconnect: missing required tool: $tool" >&2
        exit 1
    fi
done
if [ ! -f "$SOURCE_IMAGE" ] || [ ! -f kernel/Kernel64.bin ]; then
    echo "test-qemu-xhci-disconnect: image or kernel is missing" >&2
    exit 1
fi

mkdir -p "$WORK_DIR" "$ISO_ROOT/boot/grub"
touch "$SERIAL_LOG"
cp "$SOURCE_IMAGE" "$RAW_IMAGE"
cp kernel/Kernel64.bin "$ISO_ROOT/vmdex"
printf '%s\n' 'set timeout=0' \
    "menuentry \"ics\" { multiboot2 /vmdex xhci-$TEST_MODE-test; boot }" \
    > "$ISO_ROOT/boot/grub/grub.cfg"
grub-mkrescue -o "$BOOT_ISO" "$ISO_ROOT" >/dev/null 2>&1

"$QEMU_BIN" -machine q35 -smp 2 -nographic -no-reboot -m 128M \
    -cdrom "$BOOT_ISO" -boot d \
    -drive if=none,id=stick,format=raw,file="$RAW_IMAGE" \
    -device qemu-xhci,id=xhci \
    -device usb-storage,id=stickdev,bus=xhci.0,drive=stick \
    -qmp "unix:$QMP_SOCKET,server=on,wait=off" \
    < /dev/null > "$SERIAL_LOG" 2>&1 &
QEMU_PID=$!

if [[ "$TEST_MODE" == hotplug* ]]; then
    READY_MARKER='XHCI_HOTPLUG_MONITOR_READY'
else
    READY_MARKER='XHCI_DISCONNECT_INFLIGHT'
fi
if ! timeout 60 grep -a -m1 -q "$READY_MARKER" \
    < <(tail -n +1 -F "$SERIAL_LOG" 2>/dev/null); then
    echo "test-qemu-xhci-disconnect: readiness marker timed out" >&2
    tail -n 100 "$SERIAL_LOG" >&2 || true
    exit 1
fi

python3 - "$QMP_SOCKET" <<'PY'
import json
import socket
import sys

sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
sock.connect(sys.argv[1])
stream = sock.makefile("rwb", buffering=0)

def receive_response(wait_event=None):
    returned = False
    event_seen = False
    while True:
        message = json.loads(stream.readline())
        if message.get("event") == wait_event:
            event_seen = True
        if "error" in message:
            raise RuntimeError(message["error"])
        if "return" in message:
            returned = True
        if returned and (wait_event is None or event_seen):
            return

json.loads(stream.readline())
stream.write(b'{"execute":"qmp_capabilities"}\n')
receive_response()
stream.write(b'{"execute":"device_del","arguments":{"id":"stickdev"}}\n')
receive_response("DEVICE_DELETED")
stream.close()
sock.close()
PY

if [[ "$TEST_MODE" == *reconnect* || "$TEST_MODE" == mounted-remount ||
            "$TEST_MODE" == hotplug* ]]; then
        if [[ "$TEST_MODE" == hotplug* ]]; then
        RECONNECT_READY='XHCI_HOTPLUG_DISCONNECT_OK'
    else
        RECONNECT_READY='XHCI_RECONNECT_READY'
    fi
    if ! timeout 20 grep -a -m1 -q "$RECONNECT_READY" \
        < <(tail -n +1 -F "$SERIAL_LOG" 2>/dev/null); then
        echo "test-qemu-xhci-disconnect: reconnect marker timed out" >&2
        tail -n 100 "$SERIAL_LOG" >&2 || true
        exit 1
    fi
    if [ "$TEST_MODE" = reconnect-mismatch ]; then
        cp "$RAW_IMAGE" "$RECONNECT_IMAGE"
        truncate -s 64M "$RECONNECT_IMAGE"
        elif [ "$TEST_MODE" = reconnect-identity-mismatch ] ||
            [ "$TEST_MODE" = hotplug-identity-mismatch ]; then
        cp "$RAW_IMAGE" "$RECONNECT_IMAGE"
        python3 - "$RECONNECT_IMAGE" <<'PY'
import pathlib
import struct
import sys

path = pathlib.Path(sys.argv[1])
image = bytearray(path.read_bytes())
partition_lba = struct.unpack_from("<I", image, 446 + 8)[0]
boot = partition_lba * 512
if image[boot + 66] == 0x29:
    serial = boot + 67
elif image[boot + 38] == 0x29:
    serial = boot + 39
else:
    raise RuntimeError("FAT volume serial not found")
image[serial] ^= 0x5A
path.write_bytes(image)
PY
    else
        RECONNECT_IMAGE="$RAW_IMAGE"
    fi
    python3 - "$QMP_SOCKET" "$RECONNECT_IMAGE" <<'PY'
import json
import socket
import sys

sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
sock.connect(sys.argv[1])
stream = sock.makefile("rwb", buffering=0)

def receive_response():
    while True:
        message = json.loads(stream.readline())
        if "return" in message or "error" in message:
            if "error" in message:
                raise RuntimeError(message["error"])
            return

json.loads(stream.readline())
stream.write(b'{"execute":"qmp_capabilities"}\n')
receive_response()
blockdev_add = {
    "execute": "blockdev-add",
    "arguments": {
        "driver": "raw",
        "node-name": "stick2",
        "file": {"driver": "file", "filename": sys.argv[2]},
    },
}
stream.write((json.dumps(blockdev_add) + "\n").encode())
receive_response()
stream.write(b'{"execute":"device_add","arguments":{"driver":"usb-storage","id":"stickdev2","bus":"xhci.0","drive":"stick2"}}\n')
receive_response()
stream.close()
sock.close()
PY
    if [ "$TEST_MODE" = hotplug ]; then
        if ! timeout 20 grep -a -m1 -q 'XHCI_HOTPLUG_RECONNECT_OK' \
            < <(tail -n +1 -F "$SERIAL_LOG" 2>/dev/null); then
            echo "test-qemu-xhci-disconnect: first automatic reconnect timed out" >&2
            tail -n 100 "$SERIAL_LOG" >&2 || true
            exit 1
        fi
        python3 - "$QMP_SOCKET" <<'PY'
import json
import socket
import sys

sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
sock.connect(sys.argv[1])
stream = sock.makefile("rwb", buffering=0)

def receive_response(wait_event=None):
    returned = False
    event_seen = False
    while True:
        message = json.loads(stream.readline())
        if message.get("event") == wait_event:
            event_seen = True
        if "error" in message:
            raise RuntimeError(message["error"])
        if "return" in message:
            returned = True
        if returned and (wait_event is None or event_seen):
            return

json.loads(stream.readline())
stream.write(b'{"execute":"qmp_capabilities"}\n')
receive_response()
stream.write(b'{"execute":"device_del","arguments":{"id":"stickdev2"}}\n')
receive_response("DEVICE_DELETED")
stream.close()
sock.close()
PY
        if ! timeout 20 awk '/XHCI_HOTPLUG_DISCONNECT_OK/ { if (++count == 2) exit 0 }' \
            < <(tail -n +1 -F "$SERIAL_LOG" 2>/dev/null); then
            echo "test-qemu-xhci-disconnect: second automatic disconnect timed out" >&2
            tail -n 100 "$SERIAL_LOG" >&2 || true
            exit 1
        fi
        python3 - "$QMP_SOCKET" <<'PY'
import json
import socket
import sys

sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
sock.connect(sys.argv[1])
stream = sock.makefile("rwb", buffering=0)

def receive_response():
    while True:
        message = json.loads(stream.readline())
        if "error" in message:
            raise RuntimeError(message["error"])
        if "return" in message:
            return

json.loads(stream.readline())
stream.write(b'{"execute":"qmp_capabilities"}\n')
receive_response()
stream.write(b'{"execute":"device_add","arguments":{"driver":"usb-storage","id":"stickdev3","bus":"xhci.0","drive":"stick2"}}\n')
receive_response()
stream.close()
sock.close()
PY
        if ! timeout 20 awk '/XHCI_HOTPLUG_RECONNECT_OK/ { if (++count == 2) exit 0 }' \
            < <(tail -n +1 -F "$SERIAL_LOG" 2>/dev/null); then
            echo "test-qemu-xhci-disconnect: second automatic reconnect timed out" >&2
            tail -n 100 "$SERIAL_LOG" >&2 || true
            exit 1
        fi
        SUCCESS_MARKER='XHCI_HOTPLUG_RECONNECT_OK'
    elif [ "$TEST_MODE" = hotplug-identity-mismatch ]; then
        SUCCESS_MARKER='XHCI_HOTPLUG_RECONNECT_REJECTED'
    elif [ "$TEST_MODE" = reconnect-mismatch ]; then
        SUCCESS_MARKER='XHCI_RECONNECT_MISMATCH_OK'
    elif [ "$TEST_MODE" = reconnect-identity-mismatch ]; then
        SUCCESS_MARKER='XHCI_RECONNECT_IDENTITY_MISMATCH_OK'
    elif [ "$TEST_MODE" = mounted-reconnect ]; then
        SUCCESS_MARKER='XHCI_MOUNTED_RECONNECT_GENERATION_OK'
    elif [ "$TEST_MODE" = mounted-remount ]; then
        SUCCESS_MARKER='XHCI_MOUNTED_REMOUNT_OK'
    else
        SUCCESS_MARKER='XHCI_RECONNECT_OK'
    fi
else
    if [ "$TEST_MODE" = mounted-disconnect ]; then
        SUCCESS_MARKER='XHCI_MOUNTED_DISCONNECT_CACHE_OK'
    else
        SUCCESS_MARKER='XHCI_DISCONNECT_OK'
    fi
fi

if ! timeout 20 grep -a -m1 -q "$SUCCESS_MARKER" \
    < <(tail -n +1 -F "$SERIAL_LOG" 2>/dev/null); then
    echo "test-qemu-xhci-disconnect: disconnect marker timed out" >&2
    tail -n 100 "$SERIAL_LOG" >&2 || true
    exit 1
fi
if ! timeout 60 grep -a -m1 -q 'Running foreground manager thread' \
    < <(tail -n +1 -F "$SERIAL_LOG" 2>/dev/null); then
    echo "test-qemu-xhci-disconnect: post-disconnect boot timed out" >&2
    tail -n 100 "$SERIAL_LOG" >&2 || true
    exit 1
fi

kill "$QEMU_PID" 2>/dev/null || true
wait "$QEMU_PID" 2>/dev/null || true
QEMU_PID=""

grep -a -q 'xhci: configured bulk endpoints' "$SERIAL_LOG"
if [ "$TEST_MODE" = hotplug ]; then
    test "$(grep -a -c '^XHCI_HOTPLUG_DISCONNECT_OK' "$SERIAL_LOG")" -eq 2
elif [ "$TEST_MODE" = hotplug-identity-mismatch ]; then
    test "$(grep -a -c '^XHCI_HOTPLUG_DISCONNECT_OK' "$SERIAL_LOG")" -eq 1
else
    test "$(grep -a -c '^xhci: device disconnected; storage offline' "$SERIAL_LOG")" -eq 1
fi
grep -a -x -q "$SUCCESS_MARKER"$'\r' "$SERIAL_LOG"
! grep -a -q 'XHCI_DISCONNECT_FAIL' "$SERIAL_LOG"
! grep -a -q 'XHCI_MOUNTED_DISCONNECT_CACHE_FAIL' "$SERIAL_LOG"
! grep -a -q 'XHCI_MOUNTED_REMOUNT_FAIL' "$SERIAL_LOG"
! grep -a -q 'xhci: transfer timeout\|xhci: controller recovery' "$SERIAL_LOG"
! grep -a -q 'General Protection fault\|Page fault\|Double fault\|Divide by zero' "$SERIAL_LOG"
if [[ "$TEST_MODE" == mounted-* || "$TEST_MODE" == hotplug* ]]; then
    grep -a -q 'usb: registered usb0' "$SERIAL_LOG"
    grep -a -q 'Root filesystem is the USB mass-storage device' "$SERIAL_LOG"
fi
if [[ "$TEST_MODE" == mounted-* ]]; then
    grep -a -q 'usb: disconnect discarded 1 dirty cache page(s)' "$SERIAL_LOG"
elif [[ "$TEST_MODE" != hotplug* ]]; then
    ! grep -a -q 'usb: registered usb0' "$SERIAL_LOG"
fi
if [[ "$TEST_MODE" == *reconnect* || "$TEST_MODE" == mounted-remount ||
    "$TEST_MODE" == hotplug* ]]; then
    if [ "$TEST_MODE" = hotplug ]; then
        test "$(grep -a -c '^xhci: configured bulk endpoints' "$SERIAL_LOG")" -eq 3
        test "$(grep -a -c '^xhci: device re-enumerated' "$SERIAL_LOG")" -eq 2
        test "$(grep -a -c '^usb: registered block device usb0' "$SERIAL_LOG")" -eq 3
        test "$(grep -a -c '^XHCI_HOTPLUG_DISCONNECT_OK' "$SERIAL_LOG")" -eq 2
        test "$(grep -a -c '^XHCI_HOTPLUG_RECONNECT_OK' "$SERIAL_LOG")" -eq 2
    elif [ "$TEST_MODE" = hotplug-identity-mismatch ]; then
        test "$(grep -a -c '^xhci: configured bulk endpoints' "$SERIAL_LOG")" -eq 2
        test "$(grep -a -c '^xhci: device reconnect failed' "$SERIAL_LOG")" -eq 1
        test "$(grep -a -c '^usb: registered block device usb0' "$SERIAL_LOG")" -eq 1
        test "$(grep -a -c '^XHCI_HOTPLUG_RECONNECT_REJECTED' "$SERIAL_LOG")" -eq 1
        grep -a -q '^usb: replacement volume identity mismatch' "$SERIAL_LOG"
        ! grep -a -q '^XHCI_HOTPLUG_RECONNECT_OK' "$SERIAL_LOG"
    else
        test "$(grep -a -c '^xhci: configured bulk endpoints' "$SERIAL_LOG")" -eq 2
    fi
     if [ "$TEST_MODE" = reconnect-mismatch ] ||
         [ "$TEST_MODE" = reconnect-identity-mismatch ]; then
        test "$(grep -a -c '^xhci: device reconnect failed' "$SERIAL_LOG")" -eq 1
        ! grep -a -q 'xhci: device re-enumerated' "$SERIAL_LOG"
        if [ "$TEST_MODE" = reconnect-identity-mismatch ]; then
            grep -a -q '^usb: replacement volume identity mismatch' "$SERIAL_LOG"
        fi
    elif [[ "$TEST_MODE" != hotplug* ]]; then
        test "$(grep -a -c '^xhci: device re-enumerated' "$SERIAL_LOG")" -eq 1
        ! grep -a -q 'xhci: device reconnect failed' "$SERIAL_LOG"
        if [ "$TEST_MODE" = mounted-reconnect ]; then
            test "$(grep -a -c '^usb: registered block device usb0' "$SERIAL_LOG")" -eq 2
            grep -a -x -q 'XHCI_MOUNTED_RECONNECT_GENERATION_OK'$'\r' "$SERIAL_LOG"
        elif [ "$TEST_MODE" = mounted-remount ]; then
            test "$(grep -a -c '^usb: registered block device usb0' "$SERIAL_LOG")" -eq 2
            grep -a -x -q 'VFS_REMOUNT_BUSY_REJECT_OK'$'\r' "$SERIAL_LOG"
            grep -a -x -q 'XHCI_MOUNTED_REMOUNT_OK'$'\r' "$SERIAL_LOG"
        fi
    fi
fi

echo "test-qemu-xhci-$TEST_MODE PASS"
echo "Artifacts: $ARTIFACT_DIR"