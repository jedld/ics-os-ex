# AGENTS.md — ICS-OS

Guidance for humans and coding agents working in this tree.

## What this repo is

Instructional OS forked from DEX-OS. While being instructional this should not prevent this operating system from being
a state of the art, performant system and showcases the best practices in operating system design. This system should
be good enough to be operational and deployable in critical production settings.

 The active kernel path is **x86-64 long mode** (Multiboot2), with software context switching, LAPIC/SMP, ISO9660 CD root, and ELF64 user executables.

Primary code lives under `ics-os/`. Course labs are under `labs/`.

A Kernel developer guide live here, update when necessary:

wiki/Kernel-Developer's-Guide.md

## Build & test (start here)

```bash
cd ics-os
./scripts/install-deps.sh   # once
make -C kernel bzImage      # Kernel64.bin / vmdex
make -C contrib/hello install
make test-integration       # boot + SMP + exec (QEMU, serial)
```

Useful individual targets (from `ics-os/`):

| Target | Checks |
|--------|--------|
| `test-boot` | Multiboot2 ISO boots; `Root mount [OK]` |
| `test-smp` | `-smp 2`; AP online; AP work-steal; no GPF |
| `test-exec` | `hello.exe` → `Hello World` + `EXEC_TEST_PASS` |
| `test-selfhost` | In-OS TinyCC compiles/runs `min.c` + `hello.c` |
| `test-tccboot` | In-OS TinyCC rebuilds itself (`tccnew.exe`) and compiles `min.c` |
| `test-iobench` | CD sequential map; 4KiB page cache hits; `IOBENCH_PASS` + `IOBENCH_CACHE_OK` |
| `test-posixio` | POSIX fds + preadv/pwritev/fsync + io_uring; ramdisk `POSIXIO_PASS`/`URING_PASS`; virtio `/dev/vblk` `URING_VBLK_PASS` |
| `test-virtio` | QEMU virtio-blk DMA; MSI-X completions; `VIRTIO_BLK_OK` + `VIRTIO_IRQ_OK` |
| `test-spawn` | `posix_spawn` + `waitpid` of `hello.exe` (`SPAWN_PASS`); FAT `/work` on virtio (`WORK_DISK_PASS`) |
| `test-make` | In-OS TinyCC builds GNU make 3.82 onto `/work`; `make -f t.mk` spawns `hello.exe` (`MAKE_PASS`) |
| `test-bintools` | In-OS GNU binutils: `as` assembles, `ar` archives, `ld` links a default-script ELF64, then it execs (`AS_PASS`/`AR_PASS`/`LD_PASS`/`BINTOOLS_PASS`) |
| `test-integration` | `test-boot` + `test-smp` + `test-exec` |

Do **not** use QEMU `-kernel` for the ELF64 image; boot via GRUB `multiboot2` (ISO/USB helpers in the Makefile).

## Architecture notes that bite agents

- **SysV AMD64 ABI** in kernel C and IRQ wrappers (`irqwrap.S`). After `PUSH_ALL`, saved `rax` is at offset **112**, not `0` (that slot is `r15`).
- **DEX `int 0x30` ABI** still uses `rax/rbx/rcx/rdx/rsi/rdi` for syscall args; the wrapper maps them to SysV for `api_syscall`. Args are **pointer-width** (`api_arg_t`).
- **Identity map** covers low 4GiB; do not rebuild classic 2-level user PTs for that range on x86_64. PCI MMIO pages need PCD|PWT (`mmio_mark_uncacheable`); `dex32_restore_identity_map` reapplies those bits.
- **Memory map** is `kernel/memory/memlayout.h`. Kernel image must stay below 4MiB (user ELF). Frame stack follows `bssEnd`. Do not invent a new fixed PA; add a reserved range so `mempop` skips it.
- **SMP**: APs load the kernel GDT (`ap_load_kernel_gdt`), use LAPIC timer vector **0x41**, claim tasks with `on_cpu`, and honor `cpu_affinity`. Console / `fg_mgr` / user processes are BSP-pinned today.
- **Serial** is the headless oracle. Prefer `serial_puts` / putc mirroring for QEMU `-nographic` tests.
- **x86_64 in-OS TinyCC** can compile/run `min.c`/`hello.c` (`make test-selfhost`) and rebuild itself (`make test-tccboot`). TinyCC will not compile the kernel; the self-host path is TinyCC → make → binutils → GCC 4.7.4 (see `ics-os/docs/gcc-selfhost.md`).

## Coding conventions

- Match existing style: K&R-ish C, `DWORD`/`uintptr` mix, minimal new abstractions.
- Prefer small, targeted fixes over refactors. Do not reformat unrelated files.
- Kernel objects: freestanding (`-ffreestanding -fno-pic -mno-red-zone -mcmodel=large`).
- User apps: `sdk/app.mk` (`-m64`, link `crt1.c` + `tccsdk.c`).
- Document non-obvious long-mode/SMP behavior in `ics-os/docs/smp-longmode.md`.
- Ensure tests are created so that there are no regressions.

## Design considerations

- When designing system architectures, use state-of-the-art, best practices and/or industry standard mechanisms. Ensure the system is easy to use, stable and intuitive in its design. This system will eventually be used in datacenters and production grade use cases so take this into consideration.

- While Intel x86 32-bit and 64-bit amd64 is the focus at the moment, do consider design that does not impede support for other processors and systems like arm/arm64 that may be incorporated in the future.

- backwards compatibility from a user application prespective is not a concern at the moment, however if there are necessary kernel changes (e.g. system call enhancements or redesign)
that will affect applications, ensure the sample applications, SDK and tools that are provided in this repository are appropriately maintained to support these changes.

- When there are system related errors encountered during development - ensure the debugging, introspection and monitoring tools are sufficient and incorporate them as necessary. Maintainability, debugging and monitoring should be part of the feature set of the operating system.

## Research and Tools

For research on documentation, third party sources, state-of-the-art and industry standard approaches you may perform web search as available
in the current agent MCP functions as needed. You may also download any public, non-proprietary
documentation or sources for reference (e.g. hardware specs, standards). Place all of these artifacts in the /references folder for later reference and retrieval, but do not commit this as part of the repo.

Properly index these files as needed in a file called reference.md

## Development blog

Use develpment_blog.md as your diary of activities, what was done, difficulties faced and solutions to problems. Organize this by hour and date.

Make sure it contains the current problem and the activity currently being performed to solve it.

## Do not commit

- Build products: `*.o`, `Kernel64.bin`, `Kernel64.sym`, `vmdex`, `vmdex-raw`, ISO/USB images.
- Secrets or machine-local paths.

## Suggested next work

1. GCC self-host (see `ics-os/docs/gcc-selfhost.md`): in-OS TinyCC builds GNU make (`make test-make`); in-OS GNU binutils `as`/`ar`/`ld` now build **and** link+run a real ELF64 (`make test-bintools`). Next: GCC 4.7.4 (C only), which compiles ICS-OS. TinyCC `test-kbuild` is deferred.
2. Richer io_uring (registered buffers, linked SQEs) if needed. Async virtio CQEs and `/dev/vblk` are in. `posix_spawn` / `waitpid` / `/work` are in (`make test-spawn`).
3. Allow user processes on any CPU (harden `waitpid`/exit migration). Fix `fork` for x86_64 (today `posix_spawn` wraps GCC `pexecute`).
4. Full ring-3 user mode (today user ELFs still enter with kernel CS).
