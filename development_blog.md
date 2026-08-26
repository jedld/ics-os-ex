# Development blog

## 2026-08-23 (Manila, UTC+8)

### 19:00–20:30 — "Boot sometimes hangs at a different point every time" (DIAGNOSTIC MODE)

Task: boot is non-deterministic — sometimes it stops in `taskswitcher()`
(`sw=0`, `cur=dex_kernel`), sometimes it dies right after `extension:
changing schedulers..`, sometimes it goes further. Identify and fix the
root cause so the in-OS TinyCC self-host boot path is reachable.

Method: instrumented with serial diagnostics (switch counter + ready-list
dump in `ps_switchto`, `TIMER=` heartbeat in `schedule_from_timer`,
`sigwait`/`ticks`/`processmgr_busy`/`ctx_load_in_progress` GDB probes) and
used QEMU + `gdb` (`(gdb) target remote 127.0.0.1:1234`) to capture the
exact CPU state at hang time. GDB backtraces were decisive.

### 20:30–21:15 — ROOT CAUSE #1: BSS is never zeroed (THE bug)

GDB proved the freeze was an infinite busy-wait in
`sync_entercrit(&processmgr_busy)` (extension.c:73), reached via
`ps_scheduler_install()` inside `process_init()` (process.c:2151):

```
#0 lapic_read / #5 getprocessid / #6 sync_entercrit (spin: var->busy != 0)
#7 extension_override / #8 ps_scheduler_install / #9 process_init
   ticks = 0 (timer never started), processmgr_busy.busy = 0x9CE000 (garbage)
```

`startup.S` never zeroed the NOBITS `.bss` region (GRUB only loads through
`dataEnd`), so every uninitialized global held stale RAM. `processmgr_busy`
was garbage, and `process_init` only zeroed it *after* `ps_scheduler_install()`
already called `sync_entercrit()` on it → `while (var->busy && var->busy !=
getprocessid())` never terminates. Whether it hangs depends on stale RAM /
stack depth → different spot every run. **This is the non-deterministic
boot hang.**

Fixes (both, defense in depth):
- `startup.S`: zero the C-global BSS region **before `call main`**.
  Gotchas hit along the way: (a) `.bss` also contains the *in-use* boot
  page tables (`boot_pml4..boot_pd3`, CR3 points here) and the boot stack
  (`0x150000..0x160000`) — zeroing from `dataEnd` destroyed paging and
  killed boot instantly; the loop must start at `boot_stack_top`.
  (b) `movq boot_stack_top, %rsi` (no `$`) is a *memory load*, not a
  constant — GDB caught the CPU stuck in the loop with garbage bounds;
  it must be `movabs $boot_stack_top, %rsi`.
- `process.c`: explicitly zero `processmgr_busy` *before*
  `ps_scheduler_install()` (the old zeroing was after).

Result: boot now reliably passes the previously-deadlocking scheduler
install and reaches the in-OS TinyCC compile.

### 21:15–21:35 — ROOT CAUSE #2: `ready_lock` was not IRQ-safe

With boot fixed, the next hang was a **spinlock re-entrancy deadlock**
caught by GDB in the timer IRQ:

```
#0 spin_lock(&ready_lock)  <- spinning
#1 scheduler()             (scheduler.c:49)
#2 bridges_link / #3 schedule_from_timer / #4 timerwrapper (IRQ)
```

`spinlock` is a plain test-and-set with no interrupt protection.
`scheduler()`/`sched_enqueue()`/`sched_dequeue()` are reached from the
timer IRQ (IF=0) *and* from voluntary `taskswitch()`/`waitpid` paths
(IF=1). A timer IRQ landing inside an outer (IF-enabled) hold of
`ready_lock` re-enters `scheduler()` and spins forever.

Fix: `scheduler.c` — wrap each `ready_lock` critical section in
`storeflags`/`stopints` … `spin_unlock`/`restoreflags` so the lock is
always held with interrupts disabled (IRQ-safe).

### 21:35–22:00 — ROOT CAUSE #3 (build system): stale `kernel32.o`

`kernel32.c` `#include`s ~40 `.c` files (incl. `process/process.c`,
`process/sync.c`, `devmgr/extension.c`, `dexapi/dex32API.c`), but the
`kernel32.o:` Makefile rule only listed a tiny subset of them as
prerequisites. So edits to those included files were **never recompiled** —
the tested binary stayed stale (heartbeat string still present in
`Kernel64.bin` after editing `process.c` proved it). This masked the
`process.c` fixes and made boot look "the same".

Fix: `kernel/Makefile` — added all `#include`d `.c` files to the
`kernel32.o` prerequisites. Verified: `touch process/process.c` now
rebuilds `kernel32.o` and relinks.

### State after fixes

Boot is now **deterministic** and reaches the in-OS TinyCC compile:
`tccboot` extracts `/ramdisk/tcc`, loads `tcc.exe`, `createprocess`
succeeds, the child is scheduled and the timer keeps firing (thousands of
ticks). Remaining (separate, deterministic) issue — *not* the reported
non-deterministic boot hang: `tcc.exe`'s first file
`open("/icsos/pre/tcc.c")` enters a voluntary
`iso9660_loaddirectory()` → `taskswitch()` polling loop (iso9660.c:49) that
never completes, so the compile does not finish. This is a distinct
disk/ISO9660 read-completion bug and the natural next investigation.

Files changed: `kernel/startup/startup.S`, `kernel/process/process.c`,
`kernel/process/scheduler.c`, `kernel/Makefile`.

### 22:00–01:30 — Self-host pipeline: I/O deadlock → red-zone corruption → full per-file compile

Continuing from the "iso9660 read-completion" lead. Booting the
`tccboot` ISO under KVM (`-smp 1`, 1 GB) and grepping the serial log
showed boot stalling right after `Initializing the disk manager...` —
the CD `reading primary volume descriptor` never appeared.

**ROOT CAUSE #4: disk-manager starvation deadlock.** `disk_mgr` is the
only thread that executes block reads, created by `createkthread()` at
priority 0; user processes get priority 1 (`createprocess`). A user
process blocked in `open()`/`read()` polls `dex32_IOcomplete()` in a
`taskswitch()` loop — but the priority scheduler always re-selects the
runnable priority-1 user process over the priority-0 `disk_mgr`, so the
queued CD/HD read never runs and boot deadlocks.
Two wrong fixes tried and reverted (both regressed boot — stalled
*earlier*, at the disk-manager stage):
1. raise `disk_mgr` to priority 1 (equal to user) — no effect;
2. make `disk_mgr` blocking (`sleep(1)` when idle) + priority 2 —
   boot stalled even earlier. Lesson: don't touch the worker's
   priority/idle loop; the busy-poll worker must keep its scheduling
   relationship with the priority-0 boot path.

**Fix (committed `d2b43fb`):** `kernel/iomgr/iosched.c` — extract the
per-request read/write into `iomgr_execjob()` and call it
**synchronously from `dex32_requestIO()` in the caller's context**
(ints stopped, `IOrequest_busy` held, task-switching disabled). The
request completes before the caller polls, so no worker is needed on
the hot path; `disk_mgr` remains a safety-net queue drain + flusher at
its original priority. This is the standard synchronous block-I/O /
blocking work-queue model and removes the dependency on worker
scheduling entirely.
Verified: boot now reaches `Root mount [OK]`, `tccboot` extracts the
full TinyCC source to `/ramdisk`, and the in-OS `tcc.exe` compiles
files `[0]..[8]`.

**ROOT CAUSE #5: in-OS `tcc.exe` red-zone corruption (bogus
"field not found").** File `[9] i386asm.c` failed with
`i386asm.c:3980: error: field not found: instr_type` — but `instr_type`
*is* a field of `ASMInstr` (the struct definition and the `pa`
declaration are both in the preprocessed source and trivially valid).
Proof the compiler source was innocent: build the *identical*
`contrib/tcc` source on the host (host libc, `-DONE_SOURCE
-DCONFIG_TCC_STATIC`) → `/tmp/tcc_host`, then compile the very same
preprocessed `i386asm.c`: **exit 0, clean object**. (The prebuilt
`tcc.exe` cannot run on the host — it uses `int $0x30` DEX syscalls —
so a host-runnable build of the same source was the only way to
differential-test.)

Mechanism: `tcc.exe` is built by gcc with the default 128-byte red
zone (`sdk/app.mk` lacked `-mno-red-zone`). It runs at ring 0 in kernel
CS; on every `int 0x30`/IRQ the CPU pushes the 40-byte hardware frame
onto the *user* RSP, then `syscallwrapper` does `PUSH_ALL` (120 bytes)
on the same RSP (`kernel/irqwrap.S`), overwriting the red zone
`[RSP-128, RSP)` where gcc keeps live spilled values. Every syscall
from tcc silently destroyed red-zone data; on the biggest file
(i386asm.c — most syscalls, longest compile) it finally corrupted
compiler state and surfaced as a bogus semantic error. (This is the
latent bug flagged in `docs/selfhost-gap-analysis.md` §2.4.)

**Fix (committed `d9c7abd`):** add `-mno-red-zone` to `APP_CFLAGS` in
`sdk/app.mk` (matches the kernel's own build flags). Rebuilt
`tcc.exe`; in-OS it now compiles **all 10** TinyCC files including
`i386asm.c`, and `tccboot` proceeds to the `tccnew.exe` link step.

**Link-step fix (committed `6e5f2d4`).** The link command compiles the
raw SDK sources (`tccsdk.c`, `posix.c`, `crt1.c`, ...) with no include
paths → `include file 'dexsdk.h' not found`. Fixed:
`stage-tcc-short.sh` stages `sdk/dexsdk.h` into the tar's `sdk/` dir,
and the link command in `tccboot_run` passes `-I/icsos/include` (ISO
root is mounted at `/icsos`, std headers staged to `/icsos/include`).

**Refactor (same commit):** `console.c` (1588 lines) was too large for
the editing tool; extracted the self-host drivers (tccboot/kbuild/
fullhost) to `console/selfhost.c` (339 lines), re-included via
`#include "selfhost.c"` (the kernel compiles console.c as one TU via
kernel32.c, so the `static` helpers and includes resolve unchanged).
`console.c` is now 1250 lines.

Files changed: `kernel/iomgr/iosched.c`, `sdk/app.mk`,
`kernel/console/console.c`, `kernel/console/selfhost.c` (new),
`scripts/stage-tcc-short.sh`.

## 2026-08-25/26 (Manila, UTC+8)

### ~04:00–07:00 — `tccboot`: tccnew.exe compiles, links, and *runs* main(); then hits a pre-existing GPF

**Context.** After the red-zone + link-path fixes (previous session), `make
test-tccboot` got past compiling all 10 TinyCC files and linking `tccnew.exe`,
but then the VM appeared to hang. This session chased that hang to two distinct
causes.

**Root cause #1 (fixed): the in-OS compiler read uninitialised RAM.**
The x86_64 ELF loader commits an 8 MiB user heap (`ELF_HEAP_COMMIT`) and a
1 MiB user stack for every process, but never zeroed them. `dex32_commitblock`
is a no-op on x86_64 (the low 4 GiB is identity-mapped), so a fresh process's
heap/stack contained *stale RAM from whatever ran there before*. In-OS TinyCC's
own heap/stack therefore started with garbage: `sbrk` returned uninitialised
memory, the compiler's internal buffers were non-deterministic, and the
generated `tccnew.exe` differed every build (entry point bounced between
`0x447B62` and other values) — occasionally producing a binary whose first
instruction was `rip=0xf` (an IVT slot), i.e. the "hang".

Fix: zero the committed initial stack+heap in `module/elf_module.c`
(`memset` of `ELF_STACK_COMMIT` + `ELF_HEAP_COMMIT`) and zero fresh `sbrk`
brk-growth in `memory/dexmem.c`. After this, `tccnew.exe` is **deterministic**
(entry stable at `0x447B62`) and **runs**: the CRT (`crt1`) starts, `main()` is
reached, and the program begins executing its body.

**Root cause #2 (pre-existing, NOT fixed — documented): a GPF from a corrupted
context/frame.** Once `tccnew.exe` actually runs (it now survives long enough
to be preempted), it faults with a General Protection Fault at a RIP *inside
its own BSS* (`rip=0x449c68`) carrying a corrupted `SS` (`0x10`/`0xdff0010`).
Caught live under GDB, the kernel backtrace is
`GPFhandler64 <- gpfwrapper <- 0x449c68 <- 0x0` with `rax == CR3`
(`0x14b000`) — the signature of an `iretq`/context-restore that popped a
corrupted `{RIP, CS, RFLAGS, RSP, SS}` frame. The same GPF signature hit
`args.exe` earlier. The underlying cause is architectural: on x86_64 every user
process shares **one identity-mapped user window** (`userheap=0x0A000000`,
`userstackloc=0x0E000000`, `syscallstack=0x09000000` — see
`memory/dexmem.h`), so kernel interrupt/syscall frames (`PUSH_ALL` in
`irqwrap.S`) and per-task stacks live in the *same shared physical RAM* as the
user stack. When one task is preempted and another runs, frames/stacks can
clobber each other, and a later restore pops garbage. Fixing this properly
needs the long-standing ring-3 / per-process page-table work (see
`docs/selfhost-gap-analysis.md` and AGENTS.md "Suggested next work" #3), which
is out of scope for a memory bug fix.

**What was added permanently:** a real `#13` GPF dumper — `GPFhandler64`
(`hardware/exceptions.c`) prints RIP, CR2, error code, CS/SS/RSP and CR3 plus
the current process, wired in from `gpfwrapper` in `irqwrap.S`. This is what
made the diagnosis above possible (the legacy dumper only showed EIP).

**Cleanup.** All temporary DIAG instrumentation from this session was removed
(the `time.c` heartbeat + stack dump, the `elf_module.c` byte-verify/segment
dump, the `ps_switchto`/`waitpid`/`createprocess` serial trace, the `crt1.c`
`CRT1-*` markers, and the `irqwrap.S` `last_irq_*` capture). The legitimate
fixes were kept: BSS zeroing at boot (`startup.S`), the `ready_lock`
IRQ-safety in `scheduler.c`, the fd/ctty/session inheritance + `usercs` in
`process.c`/`process.h`, the `ps_switchto_in_progress` reentrancy guard, the
`-mno-red-zone` app flag, and the link-path fix.

**Regression (all PASS after cleanup + rebuild):**
- `test-boot` PASS
- `test-smp` PASS
- `test-exec` PASS
- `test-selfhost` PASS (in-OS TinyCC still compiles + runs `min.c`/`hello.c`)

**Net state of `make test-tccboot`:** the pipeline now compiles all TinyCC
sources, links `tccnew.exe` deterministically, and boots it into `main()` —
a large advance. The remaining blocker is the pre-existing shared-address-space
GPF (root cause #2), which is a ring-3 / memory-isolation task, not a memory
initialisation bug.

## 2026-08-26 (Manila, UTC+8)

### 15:00–16:20 — test-tccboot: user ELF process GPFs at entry (per-process page tables fixed)

Task: the in-OS TinyCC pipeline (`make test-tccboot`) compiled and linked
`tccnew.exe`/`hello.exe` but the executed user process died with a GPF at its
ELF entry (`RIP=0x4040C6`, the real entry of `hello.exe`) — the "root cause #2"
(shared address space / per-process paging) flagged at the end of 2026-08-23.
Goal: make user ELFs run under their private PML4 so `test-tccboot` can run
user processes without clobbering the kernel/other processes.

Method: the new `GPFhandler64` dumper (added 2026-08-23) gave RIP, CR3 and the
current process. CR3 pointed at a *private* PML4, so I dumped the full
PML4→PDPT→PD→PTE chain from the kernel for the process's entry page
(`0x404000`). The block-2 (0x400000–0x5FFFFF) PTE *table* frame existed but
contained **zero present PTEs** even though the loader's segment list was
correct. That pointed at the `userpd_*` pool allocator in `memory/dexmem.c`.

### Two root causes found and fixed

1. **In-frame free list was corruptible (design flaw).** The 32MiB dedicated
   pool (`[0x6000000,0x8000000)`) kept its free list *inside the free frames*:
   each frame's first 8 bytes held the next-free pointer. But the same pool
   holds the live PML4/PDPT/PD/PTE table pages and the process code/stack/heap,
   and it is identity-mapped, so any stray write into the pool range clobbered
   the allocator's own bookkeeping — `upop()` then returned a wrong frame or
   NULL mid-load. Replaced with the **standard bitmap allocator**: a 1024-byte
   bitmap in `.bss` (bit=1 → allocated), highest-first scan, **no metadata in
   the allocated frames**, so stray frame writes can never corrupt allocator
   state. `upop`/`upush` rewritten around it (`userpd_init_frames`,
   `up_bitmap`).

2. **Leaf PTEs written without the Present bit.** `userpd_map_page()` set
   `pte[gi] = frame | attb`, but every caller passes only `PG_WR|PG_USER`
   (=6) — `PG_PRESENT` (1) was never ORed in. Every mapped page (image, heap,
   stack, syscall stack) was therefore *not present*, so the very first
   instruction fetch of the user process faulted. Fix: force `PG_PRESENT` in
   `userpd_map_page()` (`pte[gi] = frame | (attb | PG_PRESENT)`); the PD-table
   entry already used `| 0x03`. Also normalized the PD leaf-PTE physical mask
   to the canonical `0x000FFFFFFFFF000`.

**Verification (serial, QEMU):** the process now runs to completion — its
`putcEX` stream emits the contiguous `Hello World from ICS-OS!` followed by
`EXEC_TEST_PASS`. All temporary DIAG instrumentation (PML4/PTE dumps, `UPOP`
trace, `SCD` syscall trace, `elf64SEG`/`elf64POOL`/`userpdCREATE` prints,
`userpd_last_mapfail` hook) was added to find these bugs and removed again.

**Regression (all PASS after cleanup + rebuild):**
- `test-boot` PASS
- `test-smp` PASS
- `test-exec` PASS
- `test-integration` PASS
- `test-selfhost` PASS (in-OS TinyCC still compiles + runs `min.c`/`hello.c`)

**Net state of `make test-tccboot`:** the "root cause #2" blocker (user ELFs
sharing/clobbering address space, GPF at entry) is resolved — user processes
now genuinely run under a per-process private PML4 backed by private pool
frames. Any residual tccboot issue is now downstream of process execution
(e.g. the full in-OS TinyCC rebuild path), not process memory isolation.
