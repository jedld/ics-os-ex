# GUID Partition Table (GPT) support plan

Status: Phase 0 complete (2026-09-03); Phases 1-4 not started.

Scope: read-only GPT partition discovery and mounting on ATA (IDE), USB mass
storage, and virtio-blk block devices; boot/root selection from GPT disks;
USB hotplug media identity for GPT media; host-unit and QEMU test coverage.

Related designs:

- [Testing and QA modernization plan](testing-and-qa-modernization-plan.md)
- [Concurrent VFS, device, and asynchronous I/O plan](io-subsystem-modernization-plan.md)
- [Device-driver subsystem architecture](device-driver-subsystem-architecture.md)
- [SMP long mode notes](smp-longmode.md)

## 1. Executive assessment

The kernel has no GPT support. Partition handling is MBR-only and is
duplicated in two drivers:

- `kernel/hardware/ATA/ide.c` — `ide_registerpartitions()` (lines 653-706)
  reads LBA 0 and registers up to 4 primary partitions per disk (10 total in
  `ide_partitions[10]`). It does not validate the `0x55AA` signature, uses
  32-bit LBA metadata (`DWORD startlba`, `DWORD sector_size`), and
  `ide_partition_total_blocks()` returns a signed `int`.
- `kernel/hardware/usb/uhci.c` — `usb_register_partitions()` (lines 922-968)
  validates `0x55AA`, registers up to `USB_MAX_PART` (8) partitions with the
  same 32-bit metadata, and is shared by the UHCI and xHCI backends.

There is no protective-MBR handling, no GUID partition entry parsing, no
CRC-32 validation, no backup-header fallback, no extended/logical partition
support (acknowledged in `ide.c:6-7`), and no LBA48.

GPT is required for: disks beyond the 32-bit LBA (2 TiB at 512 B sectors)
limit, more than 4 primary partitions, stable partition identity for hotplug
policy, and industry-standard disk layout in production deployments. The
UEFI specification defines the format; the current MBR paths must keep
working unchanged for existing media (USB sticks built by
`scripts/mkusb.sh`, BIOS-boot FAT disks, live CDs).

### 1.1 Defects in the current path that a GPT plan must fix anyway

- IDE disk capacity comes from legacy CHS identify words
  (`ide.c:471-473`); it is zero on LBA-only drives, so `/dev/hdp*` sizes and
  any capacity check are wrong on modern hardware.
- `ide_registerpartitions()` never checks the `0x55AA` signature
  (`ide.c:664-670`); garbage in sector 0 with nonzero "type" bytes would
  register bogus partitions.
- Partition metadata and the `total_blocks` vtable callback are 32-bit
  (`dex32_devmgr.h:184`), capping usable partitions at 1 TiB even though the
  block I/O layer is already 64-bit (`iomgr/bio.h` `bio.sector` is `u64`,
  `devmgr_block_desc.read_block` takes `u64`).
- USB hotplug media identity parses the MBR (`uhci.c:759-794`,
  `usb_identity.h`); a GPT disk would be classified as unpartitioned, fall
  back to whole-disk identity, and latch spurious identity mismatches in the
  existing xHCI hotplug gates.
- Filesystem auto-detection is unimplemented (`vfs_core.c:637-642`); root
  mount hardcodes filesystem names per device class
  (`kernel32.c:783-836`), and only ext4 has an `identify` hook
  (`ext4.c:1206-1215`).
- The only CRC-32 in the kernel is `ext4_crc32c` (Castagnoli,
  `ext4.c:58`). GPT requires the standard CRC-32 (IEEE 802.3, reflected,
  init/xorout `0xFFFFFFFF`).

### 1.2 Existing GUID/UUID infrastructure to build on

- ext4 superblock UUID (16 bytes, `ext4.c:163`).
- USB per-volume identity records (type, start LBA, sector count, serial;
  `usb_identity.h`), consumed by the xHCI hotplug/identity test family.
- The block cache already accepts 512/1024/2048/4096-byte logical sectors
  (`iomgr/blkcache.c:73-76`).
- A host-native unit-test lane exists: `tests/io_p0_unit.c` compiled by
  `make test-io-unit` emits TAP 13.

### 1.3 Boot-path constraint (BIOS + protective MBR)

Under BIOS boot, firmware sees only the protective MBR on a GPT disk: a
single `0xEE` entry spanning LBA 1 to the last LBA. The Multiboot2
`boot_device` byte therefore resolves to that protective entry, and the
current `boot_device_name` derivation (`kernel32.c:377-402`) would produce a
bogus whole-disk partition name (e.g. `hdp0p0` covering the entire disk).
Root selection must detect and special-case this.

## 2. Specification basis and scope

Format: UEFI Specification, "GUID Partition Table" section (public
documentation; a copy and its section index are recorded in `reference.md`).

Layout the parser must validate:

- Protective MBR at LBA 0: `0x55AA` signature, partition entry 0 type `0xEE`
  (EFI GPT) with first LBA 1 and sector count covering the remainder;
  entries 1-3 zero.
- Primary header at LBA 1 (92 bytes): signature `EFI PART`, revision
  1.0, header size 92, header CRC-32, reserved, MyLBA == 1, AlternateLBA ==
  last LBA, FirstUsable/LastUsable LBA (u64), DiskGUID (16 bytes),
  partition-entry LBA (1), entry count (u32, 128 typical), entry size (u32,
  128 typical), partition-entry-array CRC-32.
- Partition entry array: 128-byte entries, each with type GUID (16), unique
  partition GUID (16), FirstLBA/LastLBA (u64), attributes (u64), UTF-16LE
  name (72 bytes). Attribute bit 2 (0x0000000000000002) marks a bootable
  partition.
- Backup header at the last LBA: same checks, plus AlternateLBA == 1 and the
  same DiskGUID and entry count.

Known partition type GUIDs (initial table, extendable):

| GUID | Meaning |
|---|---|
| `C12A7328-F81F-11D2-BA4B-00A0C93EC93B` | EFI System |
| `EB0D8E9A-0989-4B88-8D77-B5E1A4B9D9A4` | Microsoft Basic Data |
| `0FC63DAF-8483-4772-8E79-3D69D8477DE4` | Linux filesystem |
| `0657FD6D-A4AB-43C4-84E5-0933C84B4F4F` | Linux swap |
| `E6D6D379-F507-44C2-A23C-238F2A3DF928` | Linux LVM |
| `21686148-6449-6E6F-744E-656564456663` | BIOS Boot |
| `DE94BBA4-06D1-4D40-A16A-BFD50179D6AC` | Windows Recovery |
| `75894C1C-3A31-4367-8D01-1B98B7C33F1A` | Windows MSFT Reserved |

In scope:

- Read-only GPT discovery, validation, partition device registration, and
  filesystem mounting (FAT, ext4; ISO9660 only where meaningful).
- 64-bit LBA partition metadata.
- GPT-aware USB media identity for hotplug policy.
- Root/boot-device selection from GPT disks, including filesystem
  auto-detection for GPT candidates.
- Host-unit tests and QEMU integration gates with deterministic fixtures.

Out of scope (v1):

- Creating or writing GPT structures (no `fdisk`-equivalent tooling).
- Extended/logical MBR partitions (separate work item).
- LVM logical volumes and Btrfs.
- 4Kn (4096-byte native sector) disks — deferred; the block cache already
  accepts 4096 B sectors.
- UEFI boot path (the system boots via BIOS/GRUB Multiboot2 today).

## 3. Architecture decisions

1. **One shared parser.** New `kernel/partition/gpt.h/.c` implements
   detection, validation, and parsing. ATA and USB registration call into it;
   virtio-blk reuses it in Phase 4. This ends the two-parser duplication and
   matches the QA policy of improving shared infrastructure over adding
   conventions.
2. **One generic partition block device.** A shared partition descriptor
   (`parent device id`, `u64 startlba`, `u64 endlba`, type/GUID, name) with
   generic `read_block`/`write_block`/`total_blocks`/`get_block_size`
   callbacks (context lookup via `devmgr_getcontext()`, bounds-checked, as in
   `uhci.c:861-913`) serves both MBR and GPT partitions. The existing
   per-driver partition callbacks in `ide.c` and `uhci.c` are refactored onto
   it with unchanged device names.
3. **Widen the capacity API.** `devmgr_block_desc.total_blocks` returns `u64`
   instead of `int`. Callers to update: `dex32_devmgr.c:607-608`
   (`DEVMGR_BLOCK_INFO`), `devfs.c:78-81/198-201/327-330`,
   `console.c:681-682` (`df`), `ide.c:637-650`, `uhci.c:824-828/861-...`,
   `virtio_blk.c:839`, `ramdisk.c:54`, `floppy.c`. Implementations with
   small capacities keep their values; the type change is the fix.
4. **IDE capacity from LBA48.** `ide_uni_get_total_sectors` reads IDENTIFY
   words 100-103 (48-bit LBA sector count) with fallback to words 61-64
   (28-bit), replacing the CHS product.
5. **Fail-closed validation.** A GPT is accepted only when the primary
   header signature, header CRC, and entry-array CRC all validate and the
   geometry is sane (MyLBA == 1, FirstUsable < LastUsable, both within
   capacity, entry LBA/count/size consistent). If the primary is corrupt but
   the backup header validates, use the backup and print a warning. If both
   fail, register no GPT partitions and fall back to the MBR path if
   `0x55AA` is clean, else treat the disk as unpartitioned. Nothing is
   mounted from unvalidated metadata.
6. **Protective entry is never a partition.** The `0xEE` entry is used only
   for detection; GPT partitions are indexed by entry number (sparse),
   preserving the `hdp0pN` / `usb0pN` naming contract.
7. **Per-disk partition cap of 32.** GPT allows 128 entries; the global
   `MAXDEVICES` (255) cap and current consumer expectations argue for 32
   registered per disk, with over-cap entries logged and skipped.
8. **Filesystem auto-detection for GPT candidates.** Implement
   `vfs_mount_device(fsname = 0)` using the existing optional `identify`
   hooks; add `identify` to `fat` (BPB check) and `cdfs` (PVD at sector 16).
   ext4 already has one. Used for GPT root selection and the `/work` mount;
   the existing MBR/USB/CD root priority order is untouched for
   backward compatibility.
9. **Read-only GPT.** No code path writes to the partition table; partition
   devices inherit the parent's read-only state as today.

## 4. Implementation plan

### Phase 0 — Prerequisites (independent bug fixes and infrastructure)

Each item ships with its own regression coverage; none changes GPT behavior.

1. Standard CRC-32 (IEEE) implementation in a small shared module
   (table-driven), distinct from `ext4_crc32c`.
2. u64 partition metadata: partition structs in `ide.c` and `uhci.c` move to
   `u64 startlba/endlba`; `total_blocks` vtable widens to `u64` across all
   implementers and callers (Section 3.3).
3. IDE capacity from LBA48 identify words (Section 3.4).
4. `ide_registerpartitions` validates `0x55AA` before parsing entries.

Verification: existing gates unchanged (`test-boot`, `test-smp`, `test-exec`,
`test-integration`, `test-usb-storage*`, `test-iobench`, `test-ext4`,
`test-spawn`, `test-virtio`, `test-posixio`), plus new host-unit cases for
CRC-32 vectors and the capacity logic (`test-partition-unit`).

Completed 2026-09-03:

- `kernel/partition/crc32.h/.c`: table-driven IEEE CRC-32, wired into
  `kernel32.c` and `kernel/Makefile`.
- `kernel/hardware/ATA/ata_capacity.h`: pure `ata_capacity_sectors()` decode
  of LBA48 (words 100-103) / LBA28 (words 60-61) from an identify image.
- `ide.c`: identify struct extended to word 103; `total_sectors` from LBA
  fields in `ide_interpret_config`; `ide_uni_get_total_sectors` and partition
  math widened to `u64`; `ide_registerpartitions` validates `0x55AA`.
- `total_blocks` vtable widened to `u64` (`dex32_devmgr.h`) with a 64-bit
  bridge (`bridges_call64`/`bridges_link64`) because the legacy `DWORD`
  bridge truncates the upper 32 bits on x86-64; implementers widened in
  `ide.c`, `uhci.c`, `virtio_blk.c`, `ramdisk.c`, `floppy.c`; callers in
  `devfs.c` and the `df` command use `bridges_call64`.
- `tests/partition_unit.c` + root `Makefile` target `test-partition-unit`
  (TAP 13, 15 checks: CRC vectors/chunking, LBA28/LBA48 decode, precedence,
  masking, 2^32 round-trip).
- Fixed a pre-existing xHCI reconnect defect surfaced by the gates: the
  "first attach" test in `usb_xhci_reconnect` keyed off publish state
  (`usb_media_established`) instead of whether media had ever been
  enumerated, so the reconnect self-tests skipped the geometry/identity
  check; it now keys off `old_blocks == 0`.

### Phase 1 — Core GPT detection and partition registration

1. `kernel/partition/gpt.h/.c`:
   - `gpt_detect()`: read LBA 0. `0x55AA` with any entry type `0xEE`
     (first LBA 1) → GPT. `0x55AA` otherwise → MBR. Neither → unpartitioned.
   - `gpt_parse()`: read primary header at LBA 1, validate (Section 3.5),
     read entry array, validate per-entry LBA bounds against the usable
     range, map type GUIDs, decode UTF-16LE names to ASCII (0x20-0x7E,
     truncating the rest), collect attributes. Backup-header fallback per
     Section 3.5.
   - Expose a read-only array: `{ index, type_guid, unique_guid, attrs,
     u64 first_lba, u64 last_lba, name[37] }` plus disk GUID, entry count,
     and a corruption-status flag for diagnostics.
2. Generic partition block device (Section 3.2) in
   `kernel/partition/`, used by both registration paths.
3. `ide_registerpartitions` (`ide.c:653`) and `usb_register_partitions`
   (`uhci.c:922`) restructured: detect → GPT path registers
   `hdp0pN` / `usb0pN` from GPT entries; MBR path keeps current behavior on
   the shared partition callbacks.
4. Capacity: `ide_partitions` grows from 10 to a per-disk table of 32;
   `USB_MAX_PART` grows from 8 to 32.
5. Console command (lsblk-style, e.g. `partitions`): for each disk print
   table type (MBR/GPT), disk GUID, and per partition: index, start/end
   (u64), size, type (name or GUID), bootable flag, UTF-16-derived name.
6. Structured boot-time diagnostics: one line per detected table
   (`GPT_DETECT <dev> entries=<n> diskguid=<...>`), one line per registered
   partition, and explicit warning lines for CRC failures and fallback use —
   these are the assertions the QEMU gates grep for.

Tests: `tests/gpt_unit.c` (host-native TAP, same lane as `test-io-unit`)
and the `test-gpt` QEMU gate (Section 5).

### Phase 2 — USB media identity and hotplug

1. `usb_capture_media_identity` (`uhci.c:759-794`) becomes table-aware: for
   GPT media it enumerates GPT entries and captures the per-volume identity
   (existing FAT/exFAT boot-sector, ext4 superblock, ISO9660 PVD readers in
   `usb_identity.h`) at each entry's first LBA; MBR behavior is unchanged.
2. The xHCI identity-mismatch and hotplug gates gain GPT fixtures
   (Section 5). The existing MBR gates are the backward-compat contract and
   must all still pass.

### Phase 3 — Boot/root selection from GPT

1. `kernel32.c:377-402` (`boot_device_name`): when the BIOS boot partition
   is the protective `0xEE` entry, the derived name is not a real partition;
   mark the disk as the "GPT boot disk" instead of emitting a partition name.
2. Root selection (`kernel32.c:783-836`): keep the existing MBR/USB/CD
   priority order. Add a GPT candidate pass: for a GPT boot disk (or any
   GPT disk when no higher-priority candidate mounts), try in order —
   partitions with attribute bit 2 (bootable), EFI System partition, then
   the first partition whose filesystem `identify` succeeds — mounting with
   auto-detection (Section 3.8).
3. `/work` mount (`kernel32.c:594-629`) uses the same auto-detect path so a
   GPT ext4 or FAT partition on `vblk`/`hdp*` works.

Tests: `test-gpt-boot` (BIOS boot with the GPT disk as the boot device,
ext4 root on the second partition) and the auto-detect unit cases.

### Phase 4 — virtio-blk and documentation

1. `vblk` partition discovery via the same `gpt_parse()` with
   `vblk_total_blocks`/read_block; partition devices `vblk0pN`.
2. `test-gpt-virtio` gate (Section 5).
3. Documentation updates:
   - `wiki/Kernel-Developer's-Guide.md`: partition section (table types,
     detection order, corruption policy, naming, hotplug identity).
   - `docs/smp-longmode.md`: boot-root note for GPT disks.
   - This plan's status line, the QA plan test matrix, and the AGENTS.md
     target table (new `test-gpt*` targets).
   - `reference.md`: index the downloaded UEFI specification GPT section in
     `/references`.
4. `development_blog.md` entries throughout.

## 5. Test plan

Per the QA policy: normal, boundary, invalid-input, and failure-path cases at
the lowest practical layer; fault injection for corruption; stable test IDs,
explicit timeouts, isolated artifacts, deterministic fixtures.

### 5.1 Host-native unit tests — `make test-gpt-unit`

`tests/gpt_unit.c` (TAP 13, pattern of `tests/io_p0_unit.c`), operating on
synthetic in-memory sector images so no QEMU is needed:

- CRC-32: known vectors (empty, "123456789", 512-byte pattern).
- Detection: valid GPT, valid MBR, protective-only LBA 0 with valid GPT,
  all-zero disk, LBA 0 without `0x55AA` but valid LBA 1 header (accepted per
  UEFI robustness, warned), MBR with `0xEE` in a non-zero slot.
- Validation failures (each must reject): bad signature, bad header CRC,
  bad entry-array CRC, MyLBA != 1, FirstUsable >= LastUsable, usable range
  beyond capacity, entry FirstLBA/LastLBA outside usable range, inconsistent
  entry count/size.
- Backup-header fallback: primary header corrupted + backup valid → accepted
  from backup; both corrupted → rejected (fail-closed); DiskGUID mismatch
  between copies → rejected.
- Entries: all 128-zero array, sparse valid entries (8 partitions — beyond
  the old 4-entry MBR limit), per-entry LBA boundary at FirstUsable/
  LastUsable, over-cap (>32) handling.
- Type-GUID mapping and UTF-16LE name decoding (ASCII, high-code-point
  truncation).
- MBR regression: existing parsing behavior preserved on the same fixtures.

### 5.2 QEMU gates

Deterministic image builder: `scripts/mkgpt.sh` (sgdisk or sfdisk) producing
a GPT image with fixed disk GUID, a FAT32 partition (root candidate) and an
ext4 partition, so every gate boots a byte-identical fixture.

| Target | Setup | Assertions |
|---|---|---|
| `test-gpt` | GPT image on `-hda`, boot via CD | `GPT_DETECT`, per-partition lines, `Root mount [OK]` on `hdp0p1`, guest write/read-back on the partition |
| `test-gpt-corrupt` | fixture variants: flipped header CRC, corrupted entry array, truncated image (LastUsable beyond capacity), missing backup header | fail-closed: no GPT partitions registered, explicit warning lines, boot continues (console ready), no crash |
| `test-gpt-boot` | GPT image is the BIOS boot device (GRUB on the disk) | protective `0xEE` not mounted, ext4 root on `hdp0p2` via bootable/auto-detect, `Root mount [OK]` |
| `test-gpt-usb` | GPT image as USB mass storage (UHCI and q35 xHCI variants) | `usb0p1` registered, guest write + SCSI `SYNCHRONIZE CACHE` + host-side byte readback (existing persistent-write contract) |
| `test-usb-storage-xhci-gpt-identity-mismatch` | same-size GPT replacement with different disk/partition GUIDs | device latched offline; existing mismatch semantics preserved |
| `test-gpt-virtio` (Phase 4) | GPT image on `/dev/vblk` | `vblk0pN` registered, ext4/FAT mount at `/work` |

All existing gates (`test-boot`, `test-smp`, `test-exec`, `test-integration`,
`test-iobench`, the full `test-usb-storage*` family, `test-ext4`,
`test-spawn`, `test-fork*`, `test-posixio`) must pass unchanged on every
phase — MBR behavior is the backward-compatibility contract.

## 6. Risks and open items

- `total_blocks` widening is a kernel-wide vtable change (about ten
  implement/call sites). It is small and contained, but it touches `devfs`
  node sizing and `df` output; Phase 0 carries its own regression pass.
- Per-disk cap of 32 partitions vs GPT's 128: over-cap entries are logged
  and skipped; revisit if a workload needs more (global `MAXDEVICES` 255 is
  the hard bound).
- BIOS boot of GRUB directly from a GPT `hda` needs a BIOS Boot partition
  for chain-loading; not required while GRUB loads from CD/USB, and the
  Phase 3 protective-entry handling keeps it viable.
- 4Kn logical-sector disks are deferred; `blkcache` already accepts 4096 B.
- The UEFI specification section must be downloaded to `/references` and
  indexed in `reference.md` before Phase 1 implementation, per the research
  policy.

## 7. Definition of done

- `make test-gpt-unit` passes (TAP), and `test-gpt`, `test-gpt-corrupt`,
  `test-gpt-boot`, `test-gpt-usb` (UHCI + xHCI), the xHCI GPT
  identity-mismatch gate, and `test-gpt-virtio` all pass in QEMU.
- The full existing gate suite passes unchanged on a clean build.
- A GPT disk with >4 partitions and >2 TiB addressable LBA (sparse fixture)
  enumerates and mounts correctly in a manual QEMU session.
- Docs (developer guide, smp-longmode, QA plan, AGENTS.md target table,
  reference.md, development_blog.md) are updated; the UEFI spec reference is
  indexed.
