# ICS-OS Self-Compile / Self-Host — Gap Analysis & Plan

Goal (as stated):

1. **OS-in-OS:** ics-os can compile itself inside itself **and boot** that compiled OS.
2. **Compiler-in-OS:** ics-os can compile its own compiler (TinyCC) inside itself,
   then **use the compiled compiler** to compile itself inside itself.

This document records the current state (measured, not assumed), the root causes of
the failures, and a phased remediation plan.

---

## 1. Capability matrix (measured 2026-08-22)

| # | Capability | Test target | Status | Evidence |
|---|------------|-------------|--------|----------|
| 0 | Host build: kernel + `tcc.exe` | `make -C kernel bzImage`, `make apps` | **PASS** | `Kernel64.bin` (351 KB), `tcc.exe` (295 KB) build cleanly |
| 1 | Compile **C programs** inside OS + run | `make test-selfhost` | **FAIL** | `tcc.exe` GPFs at entry; `SELFHOST_TEST_FAIL min.c` |
| 2 | Compile **TinyCC (the compiler)** inside OS | `make test-tccboot` | **FAIL (blocked by #1)** | Never reaches `tccboot: compiling TinyCC per-file` |
| 3 | Compile **kernel** with in-OS `tcc.exe` | `make test-kbuild` | **FAIL (blocked by #1)** | Never reaches `kbuild: compiling kernel C` |
| 4 | Compile compiler **then** kernel with the new compiler | `make test-fullhost` | **FAIL (blocked by #1)** | `tccboot_run()` returns 0 immediately |
| 5 | **Boot** the kernel compiled in-OS | (no target exists) | **NOT IMPLEMENTED** | `kbuild_run` only validates the ELF64 header, never boots it |
| 6 | Ring-3 user mode | n/a | **NOT IMPLEMENTED** | User ELFs run at ring 0 in kernel CS (see §2.1) |

**Bottom line:** none of the self-hosting tests currently pass. The single
hard blocker is that **the in-OS `tcc.exe` cannot be executed at all** (#1). Once a
user ELF can run inside the OS, #2–#4 are "just" long compiles; #5 and #6 are the
genuine new work for "compile and **boot** itself".

---

## 2. Root causes

### 2.1 Blocker — user ELFs fault immediately at entry (GPF)

Observed in `test-selfhost` serial log:

```
elf64: loaded /ramdisk/tcc.exe entry=0x43E5ED
Exception -General Protection fault process=/ramdisk/tcc.exe eip=0x43e5ed loc=0x422693
```

`eip == entry` ⇒ the fault is on the **very first instruction** of the program.
`loc` is **not** the faulting address (see §2.2) — it is the RIP of the *handler*
(`0x422693` is inside `GPFhandler`/`serial_puts`).

Contributing facts (all verified in source):

- **`gpfwrapper` passes the faulting RIP, not the faulting address.**
  [`ics-os/kernel/irqwrap.S:127`](ics-os/kernel/irqwrap.S:127)
  ```asm
  gpfwrapper:
      cli
      PUSH_ALL
      movq 128(%rsp), %rdi        /* faulting RIP for GPFhandler */
      call GPFhandler
  ```
  So the `loc=` printed by
  [`ics-os/kernel/hardware/exceptions.c:177`](ics-os/kernel/hardware/exceptions.c:177)
  is the faulting RIP, not a memory address. For a #13 (GPF) the useful info is the
  **error code** (which selector / which access), and that is **discarded** (it sits
  at `120(%rsp)`). For a #14 (page fault) the address is **CR2**, which `gpfwrapper`
  also never reads (only `pfwrapper` reads CR2, at
  [`ics-os/kernel/irqwrap.S:139`](ics-os/kernel/irqwrap.S:139)).
  **We are flying blind on the actual fault cause.**

- **User processes run at ring 0 in kernel CS, with no segment reload on context
  switch.** [`ics-os/kernel/cpu/context.S:58`](ics-os/kernel/cpu/context.S:58)
  `context_load` ends with a plain `ret` (returns to the *caller's* CS/SS, i.e. the
  kernel's). The per-process `ctx.cs`/`ctx.ss` are written but **never loaded**.
  `createprocess` records `USER_CODE/USER_DATA` in `regs`
  ([`ics-os/kernel/process/process.c:442`](ics-os/kernel/process/process.c:442)) and
  `ctx.cs = SYS_CODE_SEL`
  ([`ics-os/kernel/process/process.c:460`](ics-os/kernel/process/process.c:460)), with an
  explicit comment: *"actual CS stays kernel until TSS.rsp0 + iretq ring-3 is wired"*.

- **`setgdt` is a no-op on x86_64.**
  [`ics-os/kernel/memory/dexmem.c:582`](ics-os/kernel/memory/dexmem.c:582) returns
  immediately under `__x86_64__`, so the kernel relies entirely on the static
  `gdt64` in [`ics-os/kernel/startup/startup.S:55`](ics-os/kernel/startup/startup.S:55):

  | Selector | Entry | Meaning |
  |----------|-------|---------|
  | `0x08` (SYS_CODE_SEL) | `0x00AF9A000000FFFF` | 64-bit kernel code, **4 GiB** limit, DPL0 |
  | `0x10` (SYS_DATA_SEL) | `0x00CF92000000FFFF` | 64-bit kernel data, **4 GiB** limit, DPL0 |
  | `0x18` (user code)    | `0x00AFFA000000FFFF` | 64-bit user code, DPL3 (present but unused) |
  | `0x20` (user data)    | `0x00CFF2000000FFFF` | 64-bit user data, DPL3 (present but unused) |

  The kernel segments are **large (4 GiB)**, and the identity map covers low 4 GiB
  (2 MiB pages), so `tcc.exe`'s `.text` (`0x401000`), `.rodata` and the RW `.data/.bss`
  (`0x448ee0`, bss to `0x46dee0`) are all within a 4 GiB segment **and** identity
  mapped. In principle the ELF *should* execute at ring 0.

  **Therefore the entry GPF is almost certainly one of:**
  1. A **segment-limit / access** fault on the first `push`/`mov` to the initial
     stack — i.e. the initial RSP (`userstackloc - 8`, `0x0DFFFEF8`) or a data
     access is outside what the loaded segment permits, *or*
  2. A **stack-alignment / red-zone** issue: the host `tcc.exe` is built with the
     default `-mno-red-zone`? No — `sdk/app.mk` does **not** pass `-mno-red-zone`
     (see §2.4), so TinyCC's codegen assumes a 128-byte red zone under RSP. If any
     early kernel-injected state (or an interrupt) uses that region, the first
     C instruction can corrupt RSP and fault.
  3. The **CR3 / page table** for the user process does not actually map the
     segment pages (e.g. `dex32_commitblock` under the shared `pagedir1` identity
     map does not create a usable mapping for the new process).

  Because #2.2 discards the error code / CR2, we cannot yet tell which. **Fixing
  the fault reporting is the first concrete action.**

### 2.2 Fault reporting loses the diagnostic info

- `gpfwrapper` drops the #13 error code.
- The compact serial line prints `loc=` = faulting RIP for **both** GPF and page
  fault, which is misleading (`pagefaulthandler` receives CR2, but `GPFhandler`
  receives RIP).
- No `CR2`, no error code, no CS/SS/RSP on the serial line (only in the
  `FULLSCREENERROR` VGA dump, which is not compiled in).

### 2.3 `kbuild` does not replicate the kernel build flags

The real kernel is built with
[`ics-os/kernel/Makefile:2`](ics-os/kernel/Makefile:2):
`-mcmodel=large -mno-red-zone -fno-pie -fno-pic -msse -msse2 ...`.

The in-OS `kbuild_run` compiles kernel C with
[`ics-os/kernel/console/console.c:452`](ics-os/kernel/console/console.c:452):
`tcc -c -nostdlib -w -ffreestanding -I/ramdisk/k ...` — **missing**
`-mcmodel` (defaults to `small`), `-mno-red-zone`, `-fno-pie`, `-msse/-msse2`.

Consequences:
- Kernel objects compiled with `tcc` use the **small** code model → absolute
  32-bit references that only work if the linked image stays below 2 GiB. The
  kernel links at `0x100000`, so this *may* be OK for the kernel image itself, but
  it diverges from the host build and is fragile.
- More importantly the doc
  ([`ics-os/docs/smp-longmode.md:81`](ics-os/docs/smp-longmode.md:81)) already records
  that a tcc-linked kernel **double-faults after serial init** due to a
  **red-zone / calling-convention** mismatch. That is exactly the
  `-mno-red-zone` / SysV gap.

### 2.4 Host app build does not pass `-mno-red-zone`

[`ics-os/sdk/app.mk`](ics-os/sdk/app.mk) (used to build `tcc.exe`) does **not**
pass `-mno-red-zone`. If the in-OS kernel ever delivers an interrupt/IRQ while a
TinyCC-compiled user program is running, and the kernel handler uses the 128-byte
red zone, it will clobber user data and corrupt execution. This is a latent
correctness bug for *any* user process and is a likely contributor to the §2.1
entry fault.

### 2.5 No boot loop for the in-OS-compiled kernel

`kbuild_run` ends by validating the ELF64 header
([`ics-os/kernel/console/console.c:480`](ics-os/kernel/console/console.c:480)) and
printing `KBUILD_TEST_PASS`. It **never** attempts to boot the produced
`/ramdisk/Kernel64.bin`. There is no mechanism to (a) place a freshly compiled
kernel where GRUB can find it, or (b) reset/re-enter the bootloader with it. This
is the core of requirement #1 ("compile itself and **boot** that compiled OS").

### 2.6 tccboot / fullhost are blocked, not independently broken

`tccboot_run`, `kbuild_run` and `fullhost_run`
([`ics-os/kernel/console/console.c:310`](ics-os/kernel/console/console.c:310),
[:394](ics-os/kernel/console/console.c:394),
[:493](ics-os/kernel/console/console.c:493)) all call `run_tcc` → `user_execp` to
run `tcc.exe`. Since `tcc.exe` cannot start (§2.1), they all abort at the first
compile. They cannot be validated until §2.1 is fixed. (Their *logic* — per-file
compile, link, verify — is reasonable and was clearly written to work once exec
functions.)

### 2.7 Memory / staging assumptions

- `RAMDISK_SECTORS = 16384` → **8 MiB**
  ([`ics-os/kernel/filesystem/ramdisk.c:16`](ics-os/kernel/filesystem/ramdisk.c:16)).
  TinyCC per-file objects + the kernel objects + SDK must fit in 8 MiB. This is
  tight; a full TinyCC build produces ~10 objects plus a ~300 KB `tccnew.exe`.
  Verify it fits, or grow the ramdisk.
- The staging scripts (`stage-tcc-short.sh`, `stage-kernel-src.sh`) **host-preprocess**
  the sources with `tcc -E` and pre-assemble the kernel `.S` with `gcc`, because
  in-OS tcc "cannot assemble our .S files" and header I/O from the ISO was flaky
  (GPFs). That is a real limitation: **in-OS TinyCC cannot assemble `.S`**, so the
  kernel (and anything with assembly) can only be *rebuilt*, not *bootstrapped from
  scratch*, inside the OS.

---

## 3. What "done" looks like (acceptance criteria)

**Requirement A — compile the OS inside the OS and boot it**
1. `make test-kbuild` (extended) compiles the full kernel inside ICS-OS with
   in-OS `tcc.exe`, links `Kernel64.bin`, **and boots it**, reaching
   `Root mount [OK]` on the *newly compiled* kernel.
2. The boot loop is observable in the serial log (e.g. `KBUILD_BOOT: entry` →
   new kernel's own `Root mount [OK]`).

**Requirement B — compile the compiler inside the OS, then use it**
1. `make test-tccboot` compiles `tccnew.exe` **inside** ICS-OS from source and
   proves it works (`tccnew -v`, compiles + runs `min.c`).
2. `make test-fullhost` then uses `tccnew.exe` (the compiler *compiled inside the
   OS*) to compile the kernel and boot it (i.e. A using B's compiler).
3. `make test-selfhost` (compile+run a trivial C program inside the OS) passes —
   this is the prerequisite gate for all of the above.

---

## 4. Phased remediation plan

### Phase 0 — Make the failure diagnosable (unblocks everything)
**Goal:** stop flying blind on the entry fault.

1. **Fix `gpfwrapper` to forward the #13 error code** and print CS/SS/RSP/CR2 on
   the compact serial line.
   - [`ics-os/kernel/irqwrap.S:127`](ics-os/kernel/irqwrap.S:127): read the error
     code at `120(%rsp)` into a second arg; pass both to `GPFhandler`.
   - [`ics-os/kernel/hardware/exceptions.c:28`](ics-os/kernel/hardware/exceptions.c:28):
     accept the error code, decode bit 0 (reserved), bit 1 (segment index), bit 2
     (base/limit) and print them; also read `CR2` (even for GPF, cheap) and the
     saved CS/SS/RSP.
2. Add a one-line **boot-time** serial dump of the active GDTR and the identity-map
   coverage so we can confirm segments/map at a glance.
3. Re-run `test-selfhost`; capture the *real* fault (error code / CR2). This tells
   us whether §2.1 is (a) segment, (b) page-table, or (c) red-zone/alignment.

**Deliverable:** a serial line that says exactly why `tcc.exe` faults at entry.

### Phase 1 — Get a user ELF to run (fix §2.1)
**Goal:** `make test-selfhost` → `SELFHOST_TEST_PASS`.

Depending on the Phase-0 diagnosis, apply the relevant fix (likely all of them):

1. **If it is a red-zone / calling-convention fault:** add `-mno-red-zone` to
   `sdk/app.mk` so host-built `tcc.exe` (and any user ELF) is safe under kernel
   interrupts; rebuild `tcc.exe`.
2. **If it is a page-table / CR3 fault:** audit the ELF64 load path
   ([`ics-os/kernel/module/elf_module.c:362`](ics-os/kernel/module/elf_module.c:362)) —
   it reuses the shared identity `pagedir1`. Confirm `dex32_commitblock` actually
   maps the `PT_LOAD` pages and the stack/heap into a usable PML4 for the process,
   and that `context_load` loads the right CR3
   ([`ics-os/kernel/cpu/context.S:59`](ics-os/kernel/cpu/context.S:59)).
3. **If it is a segment fault:** ensure the user ELF is entered with a CS/SS whose
   limit covers its image. Minimal fix: enter user ELFs with the **large kernel
   CS/SS** (already the case) but verify the initial RSP is inside the mapped,
   writable, segment-covered stack. (Full ring-3 is Phase 4.)
4. Re-run `test-selfhost` until `SELFHOST_TEST_PASS`.

**Deliverable:** in-OS `tcc.exe` compiles and runs `min.c` + `hello.c`.

### Phase 2 — Compile the compiler inside the OS (fix §2.6, requirement B)
**Goal:** `make test-tccboot` → `TCCBOOT_TEST_PASS`.

1. With Phase 1 done, run `tccboot`; expect per-file compiles to proceed.
2. Confirm the **8 MiB ramdisk** is large enough for all TinyCC objects +
   `tccnew.exe`; if not, raise `RAMDISK_SECTORS`
   ([`ics-os/kernel/filesystem/ramdisk.c:16`](ics-os/kernel/filesystem/ramdisk.c:16)).
3. Verify the in-OS `tcc.exe` can emit the objects the linker needs (the host
   preprocess step in `stage-tcc-short.sh` exists to avoid ISO header I/O — keep it
   until in-OS tcc header I/O is stable).
4. Confirm `tccnew.exe -v` and that it compiles + runs `min.c`.

**Deliverable:** a TinyCC binary *built inside ICS-OS* that works.

### Phase 3 — Compile the kernel inside the OS with correct flags (fix §2.3)
**Goal:** `make test-kbuild` → `KBUILD_TEST_PASS` (valid, correctly-flagged kernel).

1. Mirror the host kernel flags in `kbuild_run`
   ([`ics-os/kernel/console/console.c:452`](ics-os/kernel/console/console.c:452)):
   add `-mcmodel=large -mno-red-zone -fno-pie -fno-pic -msse -msse2` (and
   `-fno-strict-aliasing` if needed) so in-OS objects match the host build.
2. Re-verify the link (prebuilt GAS objects from `stage-kernel-src.sh`) and the
   ELF64 header check.
3. Add a **disassembly/size sanity check** (entry ≈ `0x100000+`, sections present)
   beyond the 4-byte magic.

**Deliverable:** an in-OS-compiled `Kernel64.bin` built with the same flags as the
host kernel.

### Phase 4 — Boot the in-OS-compiled kernel (requirement A) — *the new work*
**Goal:** extend `test-kbuild` / `test-fullhost` to **boot** the freshly compiled
kernel and see it reach `Root mount [OK]`.

Design (pick one, recommend **4A**):

- **4A — Two-stage via a writable medium.**
  1. The in-OS kernel writes `/ramdisk/Kernel64.bin` to a **persistent** location the
     bootloader reads (e.g. a FAT image on a second IDE disk, or the USB image's
     FAT root). The ramdisk is RAM-only and is lost on reboot, so this needs a
     real writable disk image added to the test.
  2. The test Makefile builds a small GRUB ISO/USB whose `multiboot2` target points
     at that location, then a *second* QEMU launch (after the first reboots) boots
     the new kernel. The serial log of the second launch must show
     `Root mount [OK]` from the **newly compiled** kernel.
  3. To prove it is the new kernel, stamp a build id / banner at compile time
     (e.g. `build_id = "in-OS tcc <timestamp>"`) and assert it in the second log.

- **4B — In-place warm re-entry (harder, no second VM).**
  Load the new kernel into a reserved region, disable interrupts, set up a minimal
  Multiboot2 context, and jump to its entry with a fresh page table. This is a
  large, fragile change (it is essentially a kexec). Only if 4A is insufficient.

**Deliverable:** serial evidence that the in-OS-compiled kernel boots to
`Root mount [OK]`.

### Phase 5 — Full self-host loop (requirement B end-to-end)
**Goal:** `make test-fullhost` → `FULLHOST_TEST_PASS`.

1. `tccboot_run()` produces `tccnew.exe` **inside** the OS (Phase 2).
2. `kbuild_run("/ramdisk/tccnew.exe")` compiles the kernel **with `tccnew.exe`**
   (the compiler that was itself compiled inside the OS) and boots it (Phase 4).
3. Assert the boot stamp in Phase 4 came from the `tccnew`-built kernel.

**Deliverable:** compiler compiled in-OS → used to compile the kernel in-OS → that
kernel boots. This is the literal "compile its own compiler, then use it to compile
itself".

### Phase 6 (stretch) — Ring-3 user mode (fix §2.1 properly)
User ELFs currently run at ring 0 in kernel CS. Proper isolation:
1. Wire `TSS.rsp0` + `iretq` so `context_load` drops to DPL3 using the existing
   user code/data GDT entries (`0x18`/`0x20`).
2. Give each process its own PML4 (currently `pagedir1` is shared).
3. Re-validate all of the above under ring 3.
This is the long-term correctness fix; Phases 0–5 do not depend on it.

---

## 5. Suggested order & effort

| Phase | Unblocks | Effort | Risk |
|-------|----------|--------|------|
| 0 (diagnose) | everything | S | low |
| 1 (user ELF runs) | 2,3,5 | M | med (seg vs pgtbl vs redzone) |
| 2 (tccboot) | 5 | S–M | low (logic exists) |
| 3 (kbuild flags) | 4,5 | S | low |
| 4 (boot loop) | A | L | high (bootloader/2-VM or kexec) |
| 5 (fullhost) | B | S (deps done) | med |
| 6 (ring-3) | long-term | L | high |

**Critical path:** 0 → 1 → (2 ∥ 3) → 4 → 5.

## 6. Open questions to resolve in Phase 0

1. Does `tcc.exe` fault on a **segment** (GPF err-code bits), a **page** (CR2 in
   the stack/data range), or a **corrupt RSP** (red zone)?
2. Is the in-OS `tcc.exe` the *same* binary the host built (`apps/tcc.exe`), and
   does it run when launched directly from the console (not via `selfhost`)?
3. Does the 8 MiB ramdisk actually hold a full TinyCC object set + `tccnew.exe`?
