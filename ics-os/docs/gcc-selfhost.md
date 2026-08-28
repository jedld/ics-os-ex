# GCC self-host path (ICS-OS)

**Capstone goal:** ICS-OS self-hosts a GCC build. The way to discover every OS
gap that blocks it is to run the **real toolchain in-OS** and close what's missing.

Host Linux `gcc`/`make`/`ld`/`as`/`ar` cannot run here as-is (glibc + Linux
syscalls). So "host-compiled" means **rebuilt by the host toolchain against the
ICS-OS SDK** — the same pattern as the working `apps/make.exe`.

## Approach (Phase 1: binutils)

```
host make → as, ld, ar (built vs SDK, shipped on /work) → in-OS: ar + as + ld + exec
   → (later) GCC C-only cross-build vs SDK, in-OS self-build → ICS-OS kernel
```

Phase 1 builds GNU binutils (`as`/`ld`/`ar`) with host gcc against
`sdk/include` + `tccsdk.c`/`posix.c` (same flags as `apps/make.exe`), then an
in-OS `test-bintools` runs: `ar rcs libx.a x.o`, `as prog.s -o prog.o`,
`ld -o prog.exe ...`, exec `prog.exe` and check output. Each missing POSIX
piece (getcwd, stat/access, unlink/rename, time, ...) is an OS gap to close.

Overlay: `contrib/binutils/` (config.h + Makefile, same pattern as
`contrib/gnumake/`). Sources under `references/binutils-2.X/` (not committed).

**Status:** make in-OS is green (`test-make`: in-OS TinyCC builds make, or the
host-gcc make runs a recipe — both paths PASS). `waitpid(-1)`/`WNOHANG`, `wait()`,
`posix_spawn`, `dirent`, FAT `/work`, virtio-blk are in.

**Diagnostic note:** the GPF64 handler no longer halts the VM on **user** faults —
it dumps RIP/CR2/regs then kills the child and resumes the parent (sets
`dex32_child_faulted`), mirroring the page-fault path. Kernel faults still halt.
This is how the TCC-linked make's startup GPF (`rip=0x8`, `call *%rax`, `rax=0`)
was captured.

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
