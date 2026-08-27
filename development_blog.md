# Development blog

## 2026-08-27 (Manila, UTC+8)

### 19:50 — Memory map: one table, no more clobber-by-growth

The 0x200000 frame-stack collision was the same class of bug as the
userpd owner-table-in-BSS hang: magic PAs that the kernel image grows
into. Layout is now `kernel/memory/memlayout.h`.

- Linker `ASSERT(bssEnd <= 0x3C0000)` — kernel stays below TinyCC's
  4MiB ELF window; frame stack sits in the remainder.
- Kernel dispatcher/sched/PF stacks moved into `.bss` (same as AP
  stacks). No more 0x2800000 island.
- `mempop` seeds [4MiB, 128MiB) minus a reserved-range table. Adding a
  region means adding one table entry.
- Kernel heap is a closed 32MiB window at 32–64MiB; `sbrk` identity-
  bumps `knext` and must not `mempop` (that leaked frames and let the
  heap walk out of a 4MiB hole).
- kexec staging moved to 16–32MiB. `sharedmem` moved out of the userpd
  pool (it sat at 0x7000000 inside 96–128MiB).

`test-boot`, `test-exec`, `test-virtio`, `test-iobench` PASS.

**Activity now:** layout is the source of truth. Next still P2 blk-mq or
kbuild.

### 19:30 — I/O P0 green, P1 virtio-blk green

P0 tests: `test-boot`, `test-exec`, `test-iobench` PASS. iobench
cold/warm on the CD is still ~1.1x (ISO path bypasses blkcache);
the target is a regression oracle, not a cache proof.

P1: modern virtio-pci + virtio-blk (`vblk`). One DMA request queue,
MSI-X vector 0x42, 512-byte LBAs, FEATURES_OK + DRIVER_OK. Self-test
writes/reads the last sector. `test-virtio` PASS:
`capacity=16384 sectors msix=1` and `VIRTIO_BLK_OK`. ATA PIO stays as
the non-VM fallback.

**Hang along the way:** adding `virtio_blk.o` grew kernel `.bss` past
`0x200000`, which was the free-page stack. `mempop()` metadata was
clobbered → boot stuck at `Initializing the device manager...` (first
`malloc` after `extension_init` uses a second sbrk). Fix: place the
frame stack just after linker `bssEnd`. PCI MMIO is marked PCD|PWT;
`dex32_restore_identity_map` reapplies those bits so exec does not
turn BARs write-back again.

**Activity now:** P0+P1 landed. Next is P2 (blk-mq lite + 4KiB page
cache) or `test-kbuild`.

### 18:00 — I/O P0: drop the global lock across device I/O

Approved plan: unlock the hot path before virtio. `dex32_requestIO` no
longer holds `IOrequest_busy` or `disable_taskswitching()` during
`read_block`/`write_block`. Per-device `io_devlock[]` still serializes
ATA PIO. `IOrequest.lba` is `u64`. `disk_mgr` `sleep(1)+hlt`; fclose
calls `iomgr_request_flush()`. `test-iobench` is a real QEMU target again.

**Activity now:** build kernel, `make test-boot test-exec test-iobench`.

### 17:50 — I/O architecture review

Reviewed the live I/O path (VFS → FAT/ISO → iosched → ATA PIO / UHCI
poll). It is still a 2003 single-queue, global-lock, busy-wait design.
disk_mgr is a safety-net flusher because inline `dex32_requestIO` was
required to avoid priority starvation.

Recommended path: unlock completions (P0), virtio-blk as the VM disk
(P1), blk-mq lite + 4 KiB page cache (P2), POSIX fds then io_uring (P3).
Keep FAT/ISO; do not invent a new on-disk FS first.

**Activity now:** architecture review delivered; kbuild kernel32.c
compile remains the self-host blocker.

### 15:30 — `test-tccboot` PASS

Root cause was not `s==NULL` in main. Static TinyCC EXEs still emit a
`.plt`; `fill_got()` never wrote `R_X86_64_JUMP_SLOT` / `GLOB_DAT` into
the GOT, so `call tcc_new@plt` jumped to **rip=0** (`rcx=1` from
`stdout`). Layout-dependent: some links used PC32 (old GPF in the files
loop) and some used PLT (rip=0).

Fix in `tccelf.c`:
- set `attr->got_offset` on PLT GOT entries
- `fill_got()` walks `.got->reloc` and writes `sym->st_value` into each
  JUMP_SLOT/GLOB_DAT slot

Also: `tcc.c` reloads `s` from `tcc_state` after calls; `dexsdk.h`
`size_t` is `unsigned long` on x86_64; `contrib/tcc/Makefile` depends on
`tccelf.c` (ONE_SOURCE was not rebuilding).

`test-tccboot` PASS: tccnew compiles C `main` and `min.c` (inline asm
`_start`). `test-selfhost` PASS with KVM (TCG 256M timed out in waitpid).

### 16:00 — kbuild untar: FAT 8.3 folded `ATA` to `ata`

`test-kbuild` died extracting `ksrc.tar`:
`error locating directory` on `/ramdisk/k/hardware/ATA/ataiopio.c`
right after `hardware/hardware.h` succeeded.

FAT `file12tostr()` lowercases 8.3 names into the VFS node, so mkdir
`ATA` became `ata`. `vfs_searchname` used `strcmp`, so the parent of
`ataiopio.c` was not found. Names longer than 8.3 (`irqhandlers.c`)
would also vanish on create.

Fixes:
- keep original VFS names on FAT create
- case-insensitive path walk; prefer directories when more components remain
- ramdisk clusters 2KiB (64 dirents) so `hardware/ATA` is not capped at 16
- skip VFS-illegal tar names (`system design.txt`); drop `docs/` from ksrc.tar
- stage `aptramp.o` (ISO9660 8.3 cannot store `ap_trampoline.o` without Joliet)

Untar + kasm copy now succeed. `test-kbuild` then spent 30 min at
`kbuild: compiling kernel C` and was SIGTERM'd (QEMU stdout fully
buffered, so TinyCC progress was invisible). Next: line-buffered QEMU,
compile `tcccompat.c` first, then the unity-build `kernel32.c`.

**Activity now:** diagnose in-OS tcc compile of kernel32.c.

### 14:10 — tccnew #GP: `s` lost across calls (s==NULL at files loop)

Commit `1fe0548` is on `ics-os-v2`. Next blocker is still `test-tccboot`.

In-OS `tccnew` #GPs at `s->filetype = f->type` (`rip=0x401ee6`) with
**`rax=0x4e8`** (`offsetof(TCCState, filetype)` when `s==NULL`) and
**`rcx=0xf000ff0000000000`** (kernel leftover / non-canonical filespec).
Bytes at `0x401ecc` are the filespec walk (`add %rdx,%rcx; mov (%rcx),%rax`).
`tcc_parse_args` ran (otherwise we would not reach that loop); the
TCCState pointer was not in the stack slot main reloads.

Likely TinyCC 0.9.27 left `s` in a caller-saved register across
`tcc_parse_args` / `tcc_set_output_type`. Those run long enough for a
timer IRQ at CPL0.

Fixes in flight:
- `tcc.c`: spill `s` to `.bss` (`tcc_main_state`) and reload after calls
- `x86_64-gen.c`: 128-byte frame pad so CPL0 IRQ frames cannot overlap
  rbp-relative locals
- `dexsdk.h`: `size_t` is `unsigned long` on x86_64 (was `unsigned int`)

**Activity now:** rebuild `tcc.exe`, `make test-tccboot`.

### 12:50 — tccnew still #GP; 4K PT_LOAD is not the remaining bug

`test-selfhost` still PASSes (until a later `context_load` experiment).
`test-tccboot` still FAILs at `tccnew` compiling `min.c`.

What we proved:
- TinyCC 2MiB `ELF_PAGE_SIZE` *did* collide with the user stack. Default is
  now 4KiB; tccnew PT_LOADs sit at `0x400000` / `0x44A370`, well below the
  96MiB reserved window. `args.exe` (TCC-linked + gcc SDK) runs.
- crt1 relocs in tccnew are sane: `getparameters=0x441CB4`, `strtok=0x441D3A`,
  `main=0x401A03`. GPF RIP `0x401ee6` is **inside TinyCC `main`**, ~739 bytes
  in, with `rsp=0x0dffebe0` (valid stack) and **`rcx=0xf000ff0000000000`**
  (non-canonical). `tcc-main` printf never prints, so it dies before that
  line (likely loading a string/GOT pointer).
- Zeroing caller-saved GPRs in `context_load` broke `min.exe` exec (hung
  at `execp: started pid`). Reverted.

Still open: why TinyCC-generated `main` loads a non-canonical pointer.
`test-kbuild` / `test-fullhost` remain blocked on tccboot.

**Activity now:** reverted `context_load` GPR wipe; next is GOT/.rodata reloc
in the in-OS-linked tccnew.

### 08:30 — userpd pool leak (tccboot blocker)

`test-tccboot` compiles every TinyCC file and links `tccnew.exe`, then GPFs:
`GPF64: rip=0x449ad0 cr2=0xa2f2000 proc=/ramdisk/tccnew.exe`.

Root cause: the 32MiB per-process frame pool (`[0x06000000,0x08000000)`) is
exhausted after ~5 `tcc.exe` runs. Later execs fall back to the shared
identity map (`elf64: map fail image` / `userpd map failed; shared map`).
`tccnew` then touches heap at `0xa2f2000` (past the 2MiB commit) on that
shared map and GPFs.

`userpd_free()` existed but did not actually return frames:
1. The 4-level walker could miss leaves (2MiB PS vs 4KiB PTE tables).
2. `freelinearloc()`/`getphys()` (now a real 4-level walk) `mempush()`'d
   pool frames onto the kernel free-page stack — stealing them from
   `upop()` even if the walker later ran.
3. `kill_process` gated reclaim on `ACCESS_USER` and a pointer compare
   to `pagedir1`.

Fix (standard frame-allocator practice):
- Owner bitmap: every `upop()` is billed to the owning PML4; process
  exit scans and returns **all** billed frames. No walk required.
- `freelinearloc` skips pool frames (they are not `mempop` memory).
- `kill_process` always `userpd_free`s a pool PML4 (threads share it).
- Map each ELF `PT_LOAD` separately (do not privately map TinyCC's
  2MiB `.data` alignment gap).
- `ELF_HEAP_COMMIT` 8MiB so `tccnew` min.c does not depend on the first
  sbrk/PF for `0xa2f2000`.
- `invlpg` after splitting a 2MiB identity page; stop `sbrk` from
  `memset`ing the user VA through a stale large-page TLB.

Measured after the zombie_free fix (`test-selfhost`): pool returns to
**0/8192** after every exec; PML4 `0x7FFF000` is reused.

`tccnew` then got a private PML4 but #GP(0) at `rip=0x401ecc` on
`movsbl (%rcx),%edx` (strcpy) with **rcx=0xf000ff0000000000** (non-canonical)
and **rax=0xe000194** (just above `userstackloc` 0x0E000000). TinyCC's
default 2MiB section alignment placed later PT_LOADs on the user stack
window. Link with `-Wl,-section-alignment=1000`. Also `relocate_plt()`
for static EXE that still have a `.plt`.

**Activity now:** `make test-tccboot` with 4K section alignment, then kbuild/fullhost.

### 07:30 — Self-compile / self-host: evaluate, plan, then fix

**Goals (user):**
1. ICS-OS compiles itself *inside itself* and **boots** that compiled OS.
2. ICS-OS compiles its own compiler inside itself, then uses that compiler to compile itself inside itself.

**Measured state at start of this session (not the 2026-08-22 gap analysis):**

| Capability | Target | State |
|---|---|---|
| Host kernel + `tcc.exe` | `make -C kernel bzImage`, `make apps` | PASS |
| In-OS compile+run C | `test-selfhost` | PASS (`min.c`/`hello.c`) |
| In-OS rebuild TinyCC | `test-tccboot` | FAIL — double fault during `tccgen.c` exec (`DBLFLT cr2=0xa194000`) |
| In-OS compile kernel | `test-kbuild` | logic exists; flags incomplete; **never boots** the image |
| Compiler-then-kernel | `test-fullhost` | blocked by tccboot |
| Boot in-OS kernel | none | **not implemented** (kbuild only checks ELF magic) |

**Root causes still open:**
1. `getphys()` on x86_64 **lies** (always returns identity\|present). A real user PF (unmapped heap page after a 2MiB identity block is split) is treated as present, the dump path runs on the user CR3, and the PF handler itself faults → **double fault**. This is the tccboot blocker.
2. Double-fault wrapper printed a **stack slot address**, not RIP (`leaq` vs `movq`).
3. `syscall` MSRs written with WRMSR but **EDX never set** (STAR/LSTAR garbage high half). `syscallentry` IRETQ frame is **backwards** (RIP/CS/RFLAGS) and discards the return value via POP_ALL.
4. ELF loader maps `SYSCALL_STACK` (64KiB) then `createprocess` uses `USER_SYSCALL_STACK` (512KiB) — splitting the 2MiB syscall block leaves most of it unmapped.
5. kbuild omits host flags; no kexec/boot loop; ramdisk 8MiB is tight for tccboot+kbuild together.

**Plan (execute in this session):**
1. Fix diagnostics + `getphys` 4-level walk + demand-map missing user pages + reentrant PF/DF handlers.
2. Fix WRMSR + `syscallentry` (Linux ABI → DEX) so in-OS `tccnew.exe` can use `syscall` if it does.
3. Map the full syscall stack; grow ramdisk (16MiB) and ELF heap (16MiB); fail closed if `userpd_map_region` runs out of frames.
4. kexec: load ELF64 to a staging area, trampoline copies it over 0x100000, drop to 32-bit protected mode, jump to `startup` with Multiboot2 (`kexeced` cmdline). Stamp `build_id` so the second boot is distinguishable.
5. kbuild uses the stamp + kexec; tests assert the **new** kernel's `Root mount [OK]`.
6. `test-tccboot` then `test-kbuild` then `test-fullhost`.

**Activity now:** implementing (1)–(4).

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

### 19:00–20:30 — test-tccboot: userpd pool exhausted on the 4th exec (process-exit leak fixed)

**Task:** `make test-tccboot` now gets *further* than before — the per-process
PML4 isolation works — but the in-OS TinyCC pipeline (which execs `tcc.exe`
~20 times to compile+link `tccnew.exe`) dies after a handful of execs. Symptom:
`elf64: no userpd frames for ...; shared map` (or a GPF shortly after), i.e. the
dedicated userpd frame pool is being **exhausted** because user processes never
return their frames on exit.

**Root cause (a real leak).** On x86_64 every user ELF gets a *private* PML4
(`userpd_create`) whose 4KiB frames (image, stack, heap, syscall stack, and the
PML4/PDPT/PD/PTE table pages themselves) are drawn from the dedicated 32MiB pool
(8192 frames). But neither process-exit path reclaimed them:

- `kill_process()` only called the 32-bit `dex32_freeuserpagetable()` + `mempush()`
  path, which is wrong for a 4-level private PML4 (it would 2-level-walk a 16-entry
  PML4 and corrupt memory) — and in the x86_64 build that branch was effectively a
  no-op for the private frames.
- `schedule_from_timer()`'s self-exit reaper (the path actually taken when a user
  process calls `exit()`) freed *only the PCB* (`zombie_free`) and **never** freed
  the page tables. So every exiting user process leaked its whole PML4 tree —
  for `tcc.exe` that is image+stack+heap+tables ≈ 2.4K pool frames. The 8192-frame
  pool therefore ran dry on the ~4th exec of `tcc.exe`.

**Fix (`process.c`, `process.h`).**
1. Added `free_meminfo_list()` (process.c:1369) — frees only the `process_mem`
   metadata list, never the physical frames.
2. `kill_process()`: for a private-PML4 user process, reclaim frames with
   `userpd_free(pagedirloc)` (walks the 4-level tables, returns every *private*
   pool frame, leaves shared `boot_pd1..3`/PS pages alone) and free only the
   metadata via `free_meminfo_list()`. The shared-`pagedir1` case keeps the legacy
   `freeprocessmemory()` path.
3. Self-exit reaper in `schedule_from_timer()`: same `userpd_free()` +
   `free_meminfo_list()` before `dying->on_cpu = -1`. This is the path that was
   leaking.

   *Safety:* `userpd_free` runs while the dying task is still `current_process`,
   but it only *clears* pool-bitmap bits (no zeroing, no deref of the freed frames),
   and `context_load(&readyprocess->ctx)` immediately switches CR3 to the next
   task, so the now-unmapped frames are never touched again. No double-free is
   possible (`upush` rejects non-pool frames; clearing an already-clear bit is a
   no-op). `pagedir1` (shared) is never freed.

**Verification.**
- `test-integration` PASS (boot + smp + exec).
- `test-selfhost` PASS (in-OS TinyCC compiles+runs `min.c`/`hello.c` — exercises
  the same exit/reaper path with real user ELFs).
- `test-tccboot` now runs the **entire** in-OS TinyCC pipeline — all 21
  `tcc.exe` compiles, the `tccnew.exe` link, and the control `args.exe` build+run —
  with **no** "no userpd frames" message and **no** pool-exhaustion GPF. The leak is
  gone: the pool no longer drains across ~20 execs.

**Separate, pre-existing bug now *exposed* (not caused by this fix):** the very
final step — running the in-OS-built `/ramdisk/tccnew.exe` (a 336KB binary, larger
and built by the in-OS TinyCC/linker rather than the host toolchain) — dies with
`GPF64 err=0x0 rip=0x449ad0 cr3=0x7fff000`. To diagnose, I extended `GPFhandler64`
(`hardware/exceptions.c`) with a **full register dump** (via a `gpf_regs[19]` buffer
populated in `gpfwrapper`/`irqwrap.S` right after `PUSH_ALL`) and a 32-byte code
dump at the faulting RIP. Findings:
- Registers are all *sane* (rsp/rbp/rsi/rdx/rax are valid user-window stack
  pointers ~0xdfff…, no NULL), so it is not a simple null deref.
- The faulting RIP `0x449ad0` lands in a region **past the end of `.text`** as laid
  out by the host toolchain (host `tcc.exe` `.text` ends ~0x43f000; `.data`/`.bss`
  are 0x448ee0+). In the in-OS-built `tccnew.exe` the entry is `0x447B11` and the
  fault is `0x449ad0` — both high up. The code bytes at the fault are not a clean
  decodable instruction stream at a valid boundary, which points at the **in-OS
  TinyCC linker producing a malformed/misplaced image** for this particular large
  binary (or a loader mapping issue specific to its layout) — *not* the per-process
  paging or the pool, both of which are now correct (host `tcc.exe`, the same pool,
  runs fine pids 28–31; only the in-OS-built `tccnew.exe` misbehaves).
- This is the documented "full in-OS TinyCC rebuild (`test-tccboot`) not green yet"
  item in AGENTS.md. It is downstream of the process/memory work done here and is a
  distinct codegen/linking task.

**Diagnostic code kept** (useful, low-cost, only fires on a real #GP): the
`GPFhandler64` register + code dumps. They print once per fault and help future
ring-3 debugging; they do not alter normal operation.

## 2026-08-26 (Manila, UTC+8)

### 20:50 — Linux-ABI `syscall` compat layer: implemented, then REVERTED (boot regression)

The `tccnew.exe` `#GP(0)` root cause was confirmed as the one in the todo list:
the in-OS TinyCC emits the **Linux x86-64 `syscall`** ABI, but the kernel never
programmed the `IA32_STAR`/`IA32_LSTAR`/`IA32_SYSCALL_MASK` MSRs (it only wired the
DEX `int 0x30` path). The earlier blog note attributing the fault to an "in-OS TinyCC
linker producing a malformed image past end-of-`.text`" was a **misdiagnosis**.

I built the full compat layer:
- `startup.S`: program STAR/LSTAR/SYSCALL_MASK in `long_mode_start` (RPL3 CS/SS =
  kernel selectors, since user ELFs run CPL 0 with kernel CS).
- `irqwrap.S`: `syscallentry` (PUSH_ALL, capture the Linux register set into a
  `linux_sc_cur[9]` `.bss` global, call the C dispatcher, return via IRETQ).
- `dexapi/dex32API.c`: `syscallentry64()` logging each distinct Linux number once
  (with raw registers) and mapping read/write/open/close/brk/getpid/time/getcwd/
  fstat/stat/exit onto the DEX `api_syscall` table; `-ENOSYS` otherwise.

**Outcome: it regressed `test-boot` (69 GPFs, crash in early console init,
`Dex32PutC`, non-canonical device pointer `0xc0c00000c0b000`).** Bisection found the
regression is in the syscall-support group (`irqwrap.S`/`exceptions.c`/`dex32API.c`/
`startup.S`), **not** the `process.c/h` frame-pool fix and **not** `startup.S` alone.
The fault signature shows **runtime `.text` corruption** (image bytes at `rip` are
`Dex32PutC`, memory bytes are the long-mode-enable code) plus a **re-entrant GPF
handler** (handler faults while printing → console corruption → 69-fault cascade).
The changes are interdependent so I could not isolate the single hunk in a reasonable
number of slow QEMU cycles, and per the "no regressions" rule I **reverted the whole
layer** to the clean, passing baseline (`test-boot` PASS, 0 GPF). The frame-pool leak
fix in `process.c/h` (the prior session's work) is the only kernel change I kept out of
this revert only because it is a separate, already-validated concern — it is currently
also reverted; re-apply it independently of the syscall work.

**Deferred next steps (do NOT re-land the syscall layer as one blob again):**
1. Make the GPF handler **re-entrancy-safe** first (a `volatile` flag set under
   `cli`, early-return if already handling). This is almost certainly why the cascade
   hides the true first fault.
2. Re-introduce the syscall layer in **three tested increments**, running `test-boot`
   after each: (a) MSRs only; (b) a minimal `syscallentry` that logs and returns
   `-ENOSYS`; (c) the full mapping.
3. If a `.bss` global is the culprit, relocate it to a **dedicated page-aligned
   section outside `boot_pml4..bssEnd`** (the `zero_bss` + boot page-table range),
   rather than the generic `.bss`.

## 2026-08-27 (Manila, UTC+8)

### 04:00–14:30 — Self-hosted TinyCC: land the syscall layer incrementally, then chase a double fault

Continuing the task "do not stop until the self-hosting tcc works." The prior
session had reverted the whole Linux-ABI `syscall` layer after it regressed
`test-boot`. This session re-landed it **in three tested increments** (the plan
that blog note recommended), which succeeded:

**Increment 1 — MSRs only** (`startup.S`): program `IA32_STAR`/`IA32_LSTAR`/
`IA32_SYSCALL_MASK` in `long_mode_start`. LSTAR initially a `0` placeholder.
`test-boot` PASS, 0 GPF.

**Increment 2 — minimal `syscallentry`** (`irqwrap.S`): a `syscall` entry that
`PUSH_ALL`s, captures the Linux register set, calls a C dispatcher that (for now)
just records the last `sysno` and returns `-ENOSYS`, and returns to user mode via
a pushed `IRETQ` frame (RIP=`rcx`, CS=`0x08` kernel selector, RFLAGS=`r11`).
`test-boot` PASS, 0 GPF. **This fixed the original `#GP`** — user `syscall`
instructions no longer fault.

**Stack enlargement** (`process.h`/`elf_module.c`): the user-ELF kernel
(`int 0x30`/`syscall`) stack was `SYSCALL_STACK = 0xFFFF` (64KB). The in-OS TCC
self-build overflows it, so I added `USER_SYSCALL_STACK = 0x80000` (512KB) and
passed it to `createprocess()` from `elf_module.c`. `test-integration`
(boot + SMP + exec) still PASS.

**Result of the syscall work:** `test-tccboot` (full in-OS TinyCC self-build) now
gets the host `tcc.exe` running inside the OS and it actually *compiles* the TCC
sources — `libtcc.c`, `tccpp.c`, `tccgen.c` (pids 20/21/22) — with **zero GPFs**
and zero `-ENOSYS` stalls (the host `tcc.exe` uses the DEX `int 0x30` ABI, so the
Linux-ABI handler is irrelevant to it). This is real forward progress: the OS
boot, SMP, exec, and the in-OS compiler all work up to this point.

### THE CURRENT BLOCKER: a double fault during `tccgen.c`'s exec

While starting the exec for `tccgen.c` (pid 23), the kernel takes a **double
fault** and halts. It is deterministic and **independent of the kernel-stack
size** (64KB and 512KB both double-fault), so it is not a stack-overflow.

The old `double_fault` C handler was installed *directly* as IDT vector 8 (no
assembly wrapper), so when it fired the CPU's pushed frame (RIP/CS/RFLAGS/err)
sat under its assumed C return address → the handler faulted on its own stack →
**triple fault → "system halted"**, with **no diagnostics at all**.

**Fix landed this session (diagnostics):** added a proper `doublefaultwrapper`
(`irqwrap.S`) that captures the faulting RIP, CS, RFLAGS and CR2 and calls a new
`exc_doublefault()` (`hardware/exceptions.c`) which prints
`DBLFLT: rip=.. cs=.. rflags=.. cr2=..` over serial and halts. Wired it into the
IDT in `irqhandlers.c` (replacing the bare `double_fault`).

**Captured signature** (deterministic, every run):
```
execp: starDBLFLT: rip=0xdffe808 cs=0x0008 rflags=0x2 cr2=0xa194000
```
- `cs=0x0008` = kernel CS (CPL 0) — the fault is in kernel code.
- `cr2=0xa194000` — a **page fault** on `0xa194000` (low identity-mapped region,
  ~168MB).
- `rip=0xdffe808` — **outside the kernel `.text`** (which is `0x100024`–
  `0x142869`). So the captured RIP is not a valid kernel instruction pointer: the
  faulting context is corrupted (the `execp` print "star[t]" was cut off by the
  fault).

**Interpretation:** kernel code in the `execp`/elf64-mapping path for `tccgen.c`
triggers a **page fault** (CR2 `0xa194000`), and the **page-fault handler itself
faults** (touching an unmapped/invalid address, or recursing) → double fault.
The `PF64:` line the PF handler prints on entry never appears, so the handler
dies very early (likely in `getphys()`/`mempop()`/`getvirtaddress()` on a bad
pointer, or the PF frame is itself corrupt).

**Next steps (in order):**
1. Make the **page-fault handler re-entrancy-safe** and have it print its own
   entry (RIP via `(rsp)`, CR2, the `mm` result of `getphys`) *before* any memory
   allocation, so we see exactly which PF it is handling and where it dies.
2. Verify the `execp` elf64 private-PML4 mapping for `tccgen.c` is committing the
   right regions (the prior `tccgen.c` runs mapped PML4s at `0x6217000`/
   `0x6367000`; confirm the committed page range covers what the code touches).
3. Confirm whether `0xa194000` is a kernel data address that should be mapped in
   the *user* PML4 (it is not, if it is kernel memory) — i.e. the PF is the user
   process faulting on a kernel address, which would be a loader mapping bug.
4. Once the self-build proceeds, build out the full Linux-ABI syscall mapping
   only if the in-OS `tcc` (recompiled) actually needs it.
5. Re-verify `test-integration` + `test-selfhost` stay green.

**Files touched this session:** `startup.S` (MSRs), `irqwrap.S`
(`syscallentry` + `doublefaultwrapper`), `dex32API.c` (minimal `syscallentry64`),
`process.h` (`USER_SYSCALL_STACK`), `module/elf_module.c` (use it),
`hardware/exceptions.c` (`exc_doublefault`), `hardware/chips/irqhandlers.c`
(IDT vector 8 → `doublefaultwrapper` + extern).
