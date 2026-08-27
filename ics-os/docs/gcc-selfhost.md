# GCC self-host path (ICS-OS)

TinyCC stays the **bootstrap C compiler**. It will not compile `kernel32.c`.
The intended in-OS chain is:

```
TinyCC → GNU make → binutils (as, ld, ar) → GCC 4.7.4 (C only) → ICS-OS kernel
```

GCC 4.7.4 is the last GCC that TinyCC can build (no C++). A full GCC build
needs gigabytes of disk and a driver that `fork`s `as`/`ld`.

This document is the map for that work.

**Current round:** in-OS TinyCC builds GNU make 3.82 onto `/work` (`make test-make`,
**green**). Stock make `fork`s recipes; ICS-OS uses `posix_spawn` in a patched `job.c`
until x86_64 fork works. `waitpid(-1)` / `WNOHANG`, `wait()`, and `dirent` are in.
Overlay: `contrib/gnumake/`. Stage with `scripts/stage-make-short.sh` (host `tcc -E`,
then in-OS per-file `-c` + link against host-built `sdkobj/`).

`test-make` runs the **host-gcc** `apps/make.exe` for the recipe because the
TinyCC-linked `make.exe` (in-OS tcc objects + host-gcc `sdkobj/`) still GPFs at
`rip=0x8` (mixed-object class, same as tccnew). Next: make that link run so makeboot
executes the in-OS-built make.

## Why TinyCC-kbuild is deferred

`test-kbuild` / `test-fullhost` asked in-OS TinyCC to compile the kernel.
That is the wrong compiler for `kernel32.c`. Keep TinyCC for:

- `test-selfhost` / `test-tccboot` (bootstrap C)
- later: compiling GNU make, then a C-only GCC 4.7.4

## POSIX gaps (this round)

GCC/`make` do the usual spawn:

```c
pid = fork();
if (pid == 0) { execv(path, argv); _exit(127); }
waitpid(pid, &st, 0);
```

Historic DEX `user_execp` always **waits** (spawn+join). Console and
`tccboot` depend on that — do not change it.

| Piece | Status |
|-------|--------|
| `waitpid` | DEX syscall `0xB1` → `sys_waitpid`; SDK `sys/wait.h` |
| `wait` | SDK `int wait(int*)` → `waitpid(-1, status, 0)`; needed by GNU make's blocking job path (legacy DEX `0xC` spin-wait never yielded to the child) |
| `posix_spawn` | DEX syscall `0xB2` (`sys_spawn`): load ELF, return child pid, no wait |
| `execv` / `execvp` | DEX syscall `0xB3` (`sys_execve`): same-pid image replace (steal pid, switch) |
| `fork` / `forkprocess` | Still 32-bit paging (`disablepaging` / `dex32_copy_pg`). Unsafe on x86_64. Do not use. |
| `user_execp` (`0x5B`) | Unchanged: still waits |

The ISO 9660 root is 8.3 unless Joliet is selected; `spawn.exe` is the
packed test binary (`contrib/spawntest`, console command `spawntest`).

Stock GCC `pexecute` can be wrapped with `posix_spawn` until fork is fixed.

## Disk plan

`/ramdisk` is 16 MiB FAT16 — too small for GCC/binutils trees. `vblk` is a
raw virtio-blk device.

After `virtio_blk_init` and FAT registration, the kernel probes sector 0.
If the BPB looks like FAT, it mounts at **`/work`** and prints `work: mounted`.
No vblk, or a zeroed/non-FAT image (as in `test-virtio` / `test-posixio`),
skips the mount so `test-boot` stays green.

Host side for `test-spawn`: `mkfs.vfat -F 16` a **512 MiB** image, attach
like the posixio virtio disk. 512 MiB is enough for spawn tests and a later
trimmed binutils + make tree. GCC 4.7 itself needs a bigger image in a
later round.

FAT 8.3 still bites some GCC names even with LFN on create; that is a later
FS issue.

## Later rounds (not this one)

1. Then **binutils** (`as`, `ld`, `ar`).
2. Then **GCC 4.7.4** (`--enable-languages=c --disable-multilib`).
3. That GCC compiles ICS-OS; kexec as today.

## Tests

```
cd ics-os
make -C kernel bzImage
make test-make           # MAKE_PASS (in-OS tcc → make.exe → hello.exe)
make test-spawn          # SPAWN_PASS + WORK_DISK_PASS
make test-posixio        # still green (unformatted vblk → no /work)
make test-virtio
make test-boot
```
