# ICS-OS x86-64 long mode, software scheduling, and SMP

## Overview

The kernel now boots in **x86-64 long mode** (Multiboot2), uses **software context switching** instead of hardware TSS task gates, and brings up additional cores via the **LAPIC** (QEMU `-smp N`).

## Boot path

1. GRUB loads `vmdex` with `multiboot2` (see `scripts/mkusb.sh` / `grub-mkrescue` ISO).
2. 32-bit trampoline in `startup/startup.S` builds a 4-level identity map (0–4 GiB, 2 MiB pages), enables PAE + LME + paging, and jumps to 64-bit code.
3. LAPIC is **left enabled** (no longer disabled at boot).
4. `main()` parses Multiboot2 tags (or Multiboot1 if present).

QEMU cannot `-kernel` an ELF64 Multiboot image. Prefer the ISO smoke tests:

```
cd ics-os
make
make test-usb-amd64   # grub-mkrescue + Multiboot2
make test-smp         # qemu -smp 2
```

USB images still build with `make usb` (multiboot2 in grub.cfg); ISO boot is the more reliable QEMU path today.

## Context switch

- PCB embeds `cpu_context` + `fpu_state` (`cpu/context.h`).
- `ps_switchto()` uses `context_switch` / `context_load` (`cpu/context.S`) with `fxsave`/`fxrstor`.
- Timer IRQ uses an **interrupt gate** to `timerwrapper` → `time_handler` → `schedule_from_timer`.

## Scheduler

Default scheduler is **priority round-robin** (`process/scheduler.c`):

- Higher `PCB.priority` wins.
- Equal priority: next runnable after the previous process.
- Ready-queue walks are protected with a spinlock (SMP-ready).

## SMP

- `cpu/lapic.c` — map LAPIC, EOI, timer, INIT/SIPI IPIs.
- `cpu/smp.c` + `cpu/ap_trampoline.S` — AP trampoline at `0x8000`, per-CPU stacks.
- `current_process` is per-CPU (`smp_this_cpu()->current`).
- APs come up **parked** until `smp_enable_scheduling()` after a successful root mount, then load the **kernel GDT**, arm a **LAPIC timer on vector 0x41**, and participate in scheduling.
- Ready-queue tasks are claimed via `on_cpu`; `cpu_affinity` pins console/`fg_mgr`/user processes to the BSP. APs run migratable kthreads (see `ap_work` smoke).
- The ready-queue walk skips foreign idle threads and wrong-affinity tasks.
- QEMU `-smp 2`: `make test-smp`.

## Boot root

- Multiboot EAX/EBX are saved at `0x9000` **before** early serial I/O (which clobbers `%al`).
- Multiboot2 BIOS boot-device tag is packed into Multiboot1 layout; BIOS CD (`0xE0+`) or drive 0 → `cds0`.
- Floppy driver is skipped unless booting from `fd0`.
- **ATA PIO helpers** (`repinword` / etc. in `asmlib.S`) were fixed for SysV AMD64 (port was wrongly taken from the segment arg).
- **ISO9660** `convertname` now respects `ident_length` (names are not NUL-terminated).
- QEMU Multiboot2 ISO boots reach `Root mount [OK]` via `cdfs` on `cds0` (`make test-smp` also asserts this).
- Free-page pool expanded (~120 MiB usable under 128 MiB QEMU).
- APs unpark with kernel GDT + LAPIC timer; work-steal proven via `SMP: AP work-steal OK (cpu=1)`.
- `test-exec` runs real ELF64 CRT/`hello.exe` and expects `Hello World` + `EXEC_TEST_PASS`.

## Userland / TinyCC

- Host apps build as **ELF64** (`sdk/app.mk` uses `-m64`).
- In-OS **x86_64 TinyCC** (`apps/tcc.exe`) can compile and run programs on the
  long-mode kernel. Smoke test: `make test-selfhost` (Multiboot2 ISO, `-smp 1`).
- Selfhost stages `tcc` + sources onto `/ramdisk` first so compiles do not
  depend on ATAPI mid-run (CD reads during large ELF64 user processes remain
  flaky). Asserts `SELFHOST_TEST_PASS` after compiling/running `min.c` and
  `hello.c` (tinyio/tinycrt).
- Kernel `lmodeproc` lives at `0x10000000` so it does not collide with TCC's
  default ELF `.data` window at `0x600000`.
- **ISO9660**: directory sector padding (`size==0`) advances to the next
  sector instead of ending the directory early (multi-sector dirs like
  `/src/tcc` previously hid later files).
- Full `tccboot` (rebuild TinyCC with itself): `make test-tccboot`
  (Multiboot2 ISO, KVM). Sources + SDK + `tcc.exe` are packed as one
  ustar (`tccsrc.tar`) and extracted onto `/ramdisk`. TinyCC is then
  compiled **per file** (`-DONE_SOURCE=0`) and linked to `tccnew.exe`,
  which must compile and run `min.c`. `make test-kbuild` compiles kernel
  C with in-OS tcc and links against prebuilt GAS objects. `make
  test-fullhost` is tccboot then kbuild using `tccnew.exe`. A tcc-linked
  kernel currently double-faults after serial init (red-zone / calling
  convention); the tests assert a valid ELF64 image.
- ISO9660 directory records skip sector padding so multi-sector dirs
  (e.g. `/src/tcc`) list all files.

## TTY / userland console

The interactive shell is moving out of the kernel (`console_execute` in
`kernel/console/console.c`) onto a POSIX-style tty + userland `sh.exe`.

Kernel keeps: character queues, canonical line discipline (`\b`, `\r`),
VGA (DEX DDL) and serial backends, Ctrl-C → SIGINT to the foreground pgrp,
and a fallback kernel prompt if `/icsos/apps/sh.exe` is missing.

Userland: `contrib/sh/sh.exe` reads fd 0 / writes fd 1. Unknown commands
call `sys_kcmd` so existing builtins still work while they are ported.

Command classification (`console_execute`):

- **shell builtin:** `echo`, `cd`, `set`, `exit`, `help`, `!!`, `pwd`
- **user utility (target):** `ls`/`dir`, `cat`/`type`, `cp`/`copy`, `mkdir`,
  `ps`/`procs`, `rm`/`del`
- **privileged (stay kernel syscalls):** `mount`, `umount`, `reboot`,
  `kbuild`, `loadmod`, `selfhost`, `tccboot`

F12 still switches virtual consoles; each VT has its own tty. COM1 is
`ttyS0` (serial backend) for headless tests.

**tmux-style window keys** (prefix `Ctrl-B`, same as tmux; F2/F11/F12 still work):

| After `C-b` | Action |
|-------------|--------|
| `c` | new console |
| `n` / `p` | next / previous (wraps) |
| `l` | last window |
| `0`–`9` | select window |
| `w` | window list (fg manager) |
| `x` | kill current window (not the last one) |
| `?` | help on the status line |
| `C-b` | send a literal Ctrl-B to the tty |

A blue status line on row 24 shows `[*n:name …]` while the prefix is armed.

Ring-3: user CS is recorded as `USER_CODE` (64-bit DPL=3 GDT). Software
context switch still uses kernel CS until TSS.rsp0 + `iretq` is wired;
`int 0x30` already has DPL=3.

## Key files

| Area | Path |
|------|------|
| Long-mode entry | `kernel/startup/startup.S` |
| SysV asm helpers | `kernel/startup/asmlib.S` |
| IRQ wrappers | `kernel/irqwrap.S` |
| Context switch | `kernel/cpu/context.S` |
| LAPIC / SMP | `kernel/cpu/lapic.c`, `smp.c`, `ap_trampoline.S` |
| TTY | `kernel/console/tty.c` |
| Userland shell | `contrib/sh/sh.c` |
| Linker | `kernel/lscript64.ld` |
