# AGENTS.md — ICS-OS

Guidance for humans and coding agents working in this tree.

## What this repo is

Instructional OS forked from DEX-OS. The active kernel path is **x86-64 long mode** (Multiboot2), with software context switching, LAPIC/SMP, ISO9660 CD root, and ELF64 user executables.

Primary code lives under `ics-os/`. Course labs are under `labs/`.

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
| `test-integration` | `test-boot` + `test-smp` + `test-exec` |

Do **not** use QEMU `-kernel` for the ELF64 image; boot via GRUB `multiboot2` (ISO/USB helpers in the Makefile).

## Architecture notes that bite agents

- **SysV AMD64 ABI** in kernel C and IRQ wrappers (`irqwrap.S`). After `PUSH_ALL`, saved `rax` is at offset **112**, not `0` (that slot is `r15`).
- **DEX `int 0x30` ABI** still uses `rax/rbx/rcx/rdx/rsi/rdi` for syscall args; the wrapper maps them to SysV for `api_syscall`. Args are **pointer-width** (`api_arg_t`).
- **Identity map** covers low 4GiB; do not rebuild classic 2-level user PTs for that range on x86_64.
- **SMP**: APs load the kernel GDT (`ap_load_kernel_gdt`), use LAPIC timer vector **0x41**, claim tasks with `on_cpu`, and honor `cpu_affinity`. Console / `fg_mgr` / user processes are BSP-pinned today.
- **Serial** is the headless oracle. Prefer `serial_puts` / putc mirroring for QEMU `-nographic` tests.
- **x86_64 in-OS TinyCC** can compile/run `min.c`/`hello.c` (`make test-selfhost`). Full in-OS TinyCC rebuild (`test-tccboot`) is not green yet.

## Coding conventions

- Match existing style: K&R-ish C, `DWORD`/`uintptr` mix, minimal new abstractions.
- Prefer small, targeted fixes over refactors. Do not reformat unrelated files.
- Kernel objects: freestanding (`-ffreestanding -fno-pic -mno-red-zone -mcmodel=large`).
- User apps: `sdk/app.mk` (`-m64`, link `crt1.c` + `tccsdk.c`).
- Document non-obvious long-mode/SMP behavior in `ics-os/docs/smp-longmode.md`.

## Do not commit

- Build products: `*.o`, `Kernel64.bin`, `Kernel64.sym`, `vmdex`, `vmdex-raw`, ISO/USB images.
- Secrets or machine-local paths.

## Suggested next work

1. Allow user processes on any CPU (harden `waitpid`/exit migration).
2. x86_64 TinyCC / selfhost restore.
3. Full ring-3 user mode (today user ELFs still enter with kernel CS).
4. Stabilize `disk_mgr` / richer live images for `test-iobench`.
