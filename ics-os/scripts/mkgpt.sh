#!/bin/bash
# Build a deterministic GPT disk image for the in-kernel GPT detection /
# registration gate (make test-gpt).
#
#   Fixed disk GUID (so the serial assertion is stable run-to-run).
#   Partition slot 0: 32 MiB FAT32  (Basic Data)   -> kernel hdp0p0
#   Partition slot 1: 16 MiB ext4   (Linux data)   -> kernel hdp0p1
#
# The GPT is written byte-for-byte by the embedded python3 builder because the
# kernel's gpt.c type table is an exact on-disk byte matcher; the type GUIDs
# written here are the SAME on-disk (mixed-endian) bytes the kernel compares
# against. No root required: mtools formats the FAT32 partition at a byte
# offset, and the ext4 partition is formatted in a scratch file then dd'd in
# (no loop device needed).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

IMG="${ICSOS_GPT_IMG:-ics-os-gpt.img}"
SIZE_MB="${ICSOS_GPT_SIZE_MB:-64}"
GUID="${ICSOS_GPT_DISK_GUID:-6a8b2c3d-4e5f-4a6b-8c7d-9e0f1a2b3c4d}"

# 512-byte sector layout. Slot 0 starts at 2048 (>= GPT FirstUsableLBA 34,
# which the kernel accepts: entry.first_lba >= first_usable).
P0_START=2048
P0_SECTORS=$((32 * 2048))     # 32 MiB FAT32 (partition slot 0)
P0_LAST=$((P0_START + P0_SECTORS - 1))
P1_START=$((P0_START + P0_SECTORS))
P1_SECTORS=$((16 * 2048))     # 16 MiB ext4   (partition slot 1)
P1_LAST=$((P1_START + P1_SECTORS - 1))

for tool in python3 mformat mkfs.ext4 dd; do
    command -v "$tool" >/dev/null 2>&1 || { echo "mkgpt: missing $tool" >&2; exit 1; }
done

rm -f "$IMG"
dd if=/dev/zero of="$IMG" bs=1M count="$SIZE_MB" status=none

# ---- write the GPT (protective MBR + primary/backup headers + entry arrays) -
python3 - "$IMG" "$SIZE_MB" "$GUID" "$P0_START" "$P0_SECTORS" "$P1_START" "$P1_SECTORS" << 'PY'
import sys, struct, zlib, pathlib

img_path = sys.argv[1]
size_mb  = int(sys.argv[2])
disk_guid_canonical = sys.argv[3]
p0_first = int(sys.argv[4]); p0_last = int(sys.argv[4]) + int(sys.argv[5]) - 1
p1_first = int(sys.argv[6]); p1_last = int(sys.argv[6]) + int(sys.argv[7]) - 1

# On-disk (mixed-endian) type GUIDs -- the EXACT bytes the kernel's gpt.c
# type table compares against (Basic Data and Linux data).
BASIC_DATA_OD = bytes([0xEA,0xAD,0x0A,0xEB,0xDD,0xB2,0x3F,0x4F,0x80,0xFF,0x71,0xE9,0x16,0xB3,0x9E,0x0F])
LINUX_DATA_OD = bytes([0xAF,0x3D,0xC6,0x0F,0x83,0x84,0x72,0x47,0x8E,0x79,0x3D,0x69,0xD8,0x47,0x7D,0xE4])

def canon_to_od(canon):
    b = bytes.fromhex(canon.replace("-", ""))
    return b[0:4][::-1] + b[4:6][::-1] + b[6:8][::-1] + b[8:16]

disk_guid_od = canon_to_od(disk_guid_canonical)

total      = size_mb * 2048        # 512-byte sectors
last_lba   = total - 1
num_entries= 128
entry_size = 128
array_bytes= num_entries * entry_size
array_sec  = (array_bytes + 511) // 512
first_usable = 2 + array_sec            # 34
backup_array_lba = last_lba - array_sec # 131039
last_usable  = backup_array_lba - 1     # 131038
entry_lba    = 2

def mk_entry(type_od, uniq_od, first, last, attrs, name):
    e = bytearray(entry_size)
    e[0:16]  = type_od
    e[16:32] = uniq_od
    struct.pack_into('<Q', e, 32, first)
    struct.pack_into('<Q', e, 40, last)
    struct.pack_into('<Q', e, 48, attrs)
    e[56:128] = (name.encode('utf-16-le') + b'\x00\x00')[:72].ljust(72, b'\x00')
    return bytes(e)

entries = [
    mk_entry(BASIC_DATA_OD, canon_to_od("11111111-2222-3333-4444-555555555555"), p0_first, p0_last, 0, "icsos-fat"),
    mk_entry(LINUX_DATA_OD, canon_to_od("11111111-2222-3333-4444-666666666666"), p1_first, p1_last, 0, "icsos-ext4"),
]
while len(entries) < num_entries:
    entries.append(bytes(entry_size))
entry_array = b"".join(entries)
array_crc = zlib.crc32(entry_array) & 0xffffffff

def mk_header(my_lba, alt_lba, elba):
    h = bytearray(512)
    h[0:8] = b"EFI PART"
    struct.pack_into('<I', h, 8,  0x00010000)  # revision
    struct.pack_into('<I', h, 12, 92)          # header size
    struct.pack_into('<Q', h, 24, my_lba)
    struct.pack_into('<Q', h, 32, alt_lba)
    struct.pack_into('<Q', h, 40, first_usable)
    struct.pack_into('<Q', h, 48, last_usable)
    h[56:72] = disk_guid_od
    struct.pack_into('<Q', h, 72, elba)
    struct.pack_into('<I', h, 80, num_entries)
    struct.pack_into('<I', h, 84, entry_size)
    struct.pack_into('<I', h, 88, array_crc)
    struct.pack_into('<I', h, 16, zlib.crc32(bytes(h[0:92])) & 0xffffffff)
    return bytes(h)

img = bytearray(size_mb * 1024 * 1024)
img[0:512] = b'\x00' * 512
# protective MBR
mbr = bytearray(512)
pe = bytearray(16)
pe[0] = 0x00
pe[4] = 0xEE
struct.pack_into('<I', pe, 8, 1)             # start LBA
struct.pack_into('<I', pe, 12, last_lba - 1) # sector count (LBA 1 .. last_lba-1)
mbr[446:462] = bytes(pe)
mbr[510], mbr[511] = 0x55, 0xAA
img[0:512] = bytes(mbr)
img[512:1024] = mk_header(1, last_lba, entry_lba)
img[entry_lba*512 : entry_lba*512 + array_bytes] = entry_array
img[backup_array_lba*512 : backup_array_lba*512 + array_bytes] = entry_array
img[last_lba*512 : last_lba*512 + 512] = mk_header(last_lba, 1, backup_array_lba)
pathlib.Path(img_path).write_bytes(bytes(img))
print("GPT: total=%d first_usable=%d last_usable=%d array_crc=%08x"
      % (total, first_usable, last_usable, array_crc))
PY

# FAT32 on partition slot 0. mtools addresses a raw image at a byte offset.
P0_OFF=$((P0_START * 512))
mformat -i "${IMG}@@${P0_OFF}" -F -v ICSOSGPT ::

# ext4 on partition slot 1: format a scratch file, then dd it into the image.
EXT4_TMP="$(mktemp)"
trap 'rm -f "$EXT4_TMP"' EXIT
dd if=/dev/zero of="$EXT4_TMP" bs=512 count="$P1_SECTORS" status=none
mkfs.ext4 -q -F -L icsosex4 "$EXT4_TMP"
dd if="$EXT4_TMP" of="$IMG" bs=512 seek="$P1_START" conv=notrunc status=none

chmod 666 "$IMG" 2>/dev/null || true
echo "Created $IMG (${SIZE_MB} MiB, GPT disk GUID $GUID)."
echo "  p0: FAT32  LBA ${P0_START}..${P0_LAST}  (Basic Data)"
echo "  p1: ext4   LBA ${P1_START}..${P1_LAST}  (Linux data)"
