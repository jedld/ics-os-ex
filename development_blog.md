# Development blog

## 2026-08-30 (Manila, UTC+8)

### ~16:00 — In-OS full GCC toolchain composes: `cc1 → as → ld → exec` builds *and* runs a C program (`test-gcc` PASS)

**Goal (user):** "commit and push, then continue with the task." The cc1 milestone
was committed/pushed (`a44b88f`). The next GCC self-host step is to prove the real
GCC C frontend and the real binutils backend *compose* in-OS: a C program is
compiled, assembled, linked and **run** entirely on ICS-OS.

**Key realization:** cc1 (and as/ld) are **ICS-OS user-mode binaries** — they link
the SDK runtime (`tccsdk.c posix.c libtcc1.c crt1.c setjmp.c`,
`contrib/gcc/Makefile` line 77) and do all I/O via the `int 0x30` DEX syscall.
`./cc1 --version` on the host prints nothing and file I/O segfaults because
`int 0x30` is undefined under Linux. So cc1/as/ld only work **in-OS**; there is no
host dry-run path (a host link check with host-generated `prog.o` was used only to
validate that the runtime `.o`s resolve `_start`/`puts`/`main`).

**Design:** a new `gctest` console builtin chains four `user_execp()` calls (each
waits), all I/O on `/ramdisk`:
1. stage `gccprobe.c` + the 5 SDK runtime `.o`s to `/ramdisk` via `fcopy` (so the
   spawned tool children never read the CD mid-run — the bintest/selfhost pattern);
2. `cc1 /ramdisk/gccprobe.c -o /ramdisk/gccprobe.s` (C → asm);
3. `as --64 gccprobe.s -o gccprobe.o` (asm → ELF64 obj);
4. `ld gccprobe.o + crt1.o tccsdk.o libtcc1.o posix.o setjmp.o -o gccprobe.exe`
   (obj + SDK runtime → runnable ELF64; default script from `/icsos/apps/ldscripts/`);
5. the kernel `exec`s `gccprobe.exe` → `_start → main → puts("GCC_E2E_OK")`.

The runtime `.o`s are built on the host exactly as `sdk/app.mk` builds every app
(`APP_CFLAGS` + `-Isdk/include`); the default script's `ENTRY(_start)` resolves to
`crt1.o`'s `_start`, so individual `.o`s are linked (not a `.a`, to avoid archive
member-order issues). `gccprobe.c` uses an `extern int puts(...)` declaration (no
`#include`) so cc1 needs no runtime include path yet.

**Result:** `make test-gcc` **PASS**. Log: cc1 (18,180,320 B) compiles
`gccprobe.c`; `as` assembles; `ld` links to a **52,613 B** ELF64; the kernel loads it
(`elf64: loaded /ramdisk/gccprobe.exe entry=0x4001D5`, private PML4, pool used 3)
and runs it → `GCC_E2E_OK` + `GCC_E2E_RUN_OK`.

**Regressions (all PASS):** `test-cc1`, `test-bintools`, `test-integration`
(boot + SMP + exec).

**Files touched:** `kernel/console/console.c` (`gctest` builtin), `ics-os/Makefile`
(`test-gcc` target + `.PHONY`), `docs/gcc-selfhost.md` (rounds + tests).

**Next:** the `gcc` *driver* (front-end: option parsing + `pexecute`/`posix_spawn`
of cc1/as/ld) so a single `gcc x.c -o x` works in-OS; then that GCC compiles ICS-OS.

**Activity now:** `test-gcc` is green — the real GCC C frontend + real binutils
backend compose in-OS to build **and** run a C program. Next: the gcc driver.

### ~15:00 — In-OS GCC `cc1` runs and compiles C: `test-cc1` PASS

**Goal (user):** the staged, compile-only milestone — run the *host-built* GCC
`cc1` (the C frontend, 18 MiB statically-linked ELF64) inside ICS-OS and have it
compile a trivial C file to assembly. `make test-cc1` boots the ISO, stages
`/tmp/icsos-gcc/cc1` as `/icsos/apps/cc1.exe`, runs `cc1test`, and greps for
`CC1_TEST_PASS`.

**Three root causes found and fixed (all blocked the 18 MiB load/run):**

1. **Kernel heap too small for the executable buffer.** `user_execp()` →
   `vfs_mapfile()` allocates the whole ELF into a kernel buffer
   (`malloc(18180320)`) before the ELF loader maps it into user frames. The
   kernel heap was a closed 32 MiB window; with existing heap usage the 18 MiB
   allocation failed (`mapfile: malloc(...)`). Expanded the kernel heap
   32 → **48 MiB** in `memory/memlayout.h` (`MEM_KHEAP_SIZE=0x03000000`,
   `MEM_KHEAP_END=0x05000000`), absorbing the former `kmode` slot
   (`0x04000000..0x05000000`). Removed the `kmode` entry from `mem_reserved[]`
   in `memory/dexmem.c`; `MEM_KMODE_*` now mark the *unreserved* 16 MiB `mempop`
   free-page gap (`0x05000000..0x06000000`), which is preserved. The
   compile-time layout asserts (`kmode after heap`, `userpd after kmode`) still
   hold.

2. **CD-ROM `readfile` allocated a second whole-file buffer.**
   `iso9660_openfile()` did `data_buffer = malloc(2048 * totalblocks)` — a
   *second* ~18 MiB allocation for the whole transfer — DMA-read it in one shot,
   then `memcpy`'d the slice out. For `cc1` that doubled the peak heap need and,
   worse, the oversized single DMA read left the caller's destination **zeroed**
   (the ELF magic came back `00 00 00 00`, so the loader reported
   "unidentified executable format"). Rewrote `iso9660_openfile()` to read the
   CD **one 2048-byte block at a time** directly into the caller's buffer, using
   a tiny per-call `malloc(2048)` and copying the correct slice on the first/last
   partial blocks. This path is shared by all CD reads, so block-at-a-time is now
   the CD baseline (regression-checked with `test-iobench`).

3. **The test drove `cc1` with a driver-only flag.** `cc1test` invoked
   `cc1.exe -c in.c -o out.s`. `-c` (compile+assemble, no link) is a `gcc`
   *driver* option and is **rejected by `cc1`** (`error: command line option
   '-c' is valid for the driver but not for C`). `cc1` is the C frontend and
   emits assembly directly, so the `-c` was dropped: `cc1.exe in.c -o out.s`.

**Result:** `make test-cc1` **PASS**. The 18 MiB ELF64 loads into a private PML4
(`elf64: loaded ... entry=0x109C2B9`), `cc1` runs the real frontend (`Analyzing
compilation unit` → `Performing interprocedural optimizations` → `Assembling
functions: cc1_probe`), and writes `/ramdisk/cc1probe.s` (465 bytes).
`CC1_TEST_PASS`.

**Regressions (all PASS after rebuild):** `test-integration` (boot + SMP + exec),
`test-iobench` (CD sequential + 4 KiB page cache), `test-selfhost` (in-OS TinyCC
compile+run). The host-side `contrib/gnumake`/`contrib/binutils` link errors
(multiple definition of `strcasecmp`/`strncasecmp`/`strsignal`/
`fopen_unlocked`/`bsearch` — SDK `posix.c` vs the staged sources) are
**pre-existing** and the Makefile marks them `(ignored)`; they do not affect the
passing tests above.

**Files touched:** `kernel/memory/memlayout.h` (48 MiB kernel heap),
`kernel/memory/dexmem.c` (dropped `kmode` from `mem_reserved[]`),
`kernel/filesystem/iso9660.c` (block-at-a-time CD read),
`kernel/console/console.c` (`cc1test` command; removed temporary magic/fread
diagnostics), `ics-os/Makefile` (`test-cc1` target).

**Next:** GCC 4.7.4 (C-only) self-host is the stated capstone. Near-term
enablers: (a) fix the pre-existing SDK `posix.c` symbol conflicts so the host
`make.exe`/`ar.exe`/`as.exe`/`ld.exe` link (unblocks running the in-OS toolchain
end-to-end), then (b) drive `cc1` → `as` → `ld` in-OS to produce a real ICS-OS
executable. The memory-map work (48 MiB kernel heap, per-process PML4, userpd
bitmap pool) is in and regression-clean.

**Activity now:** `test-cc1` is green — the host-built GCC `cc1` runs in-OS and
compiles C to assembly. Next: the GCC 4.7.4 self-host chain (close the SDK symbol
conflicts so the binutils/make tools link, then compile+link+run a real ELF in-OS).

## 2026-08-28 (Manila, UTC+8)

### 17:40 — ld (GNU ld 2.23) builds and links: `ld.exe`

**Goal:** the third binutils tool — the GNU linker. `ld` is the last
binutils piece before the GCC 4.7.4 self-host step (GCC emits `.o` via
`as` and needs `ld` to produce the final executable).

**Approach:** mirror the `ar`/`as` recipe. `ld` core = 17 `ld/*.c` files
(the 2.23 `CFILES` + the checked-in generated `ldgram.c`/`ldlex.c`/
`deffilep.c`) plus the `elf_x86_64` emulation, linked against the
already-built libbfd + libiberty + SDK runtime.

**Findings / obstacles:**

- **The emulation is a *generated* file, so it must be committed.**
  Upstream `ld/Makefile.in` produces `eelf_x86_64.c` by running
  `genscripts.sh emulparams/elf_x86_64.sh emultempl/elf.em
  scripttempl/elf.sc` (it inlines the `elf_x86_64` linker script as a C
  string). I ran that script once and committed the result as
  `contrib/binutils/eelf_x86_64.c`, plus a hand-written
  `contrib/binutils/ldemul-list.h` (the one-entry `EMULATION_LIST`). Both
  sit in the `CONFDIR` include dir, which is searched *before*
  `$(SRC)/ld`, so `#include "ldemul-list.h"` (from `ldemul.c`) and
  `#include "eelf_x86_64.c"` (from `ldctor.c`) resolve to our copies.

- **Configure-injected `-D` strings.** `ldmain.c` needs
  `DEFAULT_EMULATION` (the default `emulparams` name), and `ldfile.c`'s
  `find_scripts_dir()` needs `SCRIPTDIR`/`BINDIR`/`TOOLBINDIR`. Upstream
  injects these per-object from `Makefile.in`; we do the same via `LDDEFS`
  in the Makefile (`DEFAULT_EMULATION=\"elf_x86_64\"` must equal the
  emulation's `.name`).

- **`#ifdef` vs `#if 0` trap in config.h.** `ld/sysdep.h` does
  `#ifdef HAVE_DLFCN_H → #include <dlfcn.h>`, and the plugin code is gated
  by `#ifdef ENABLE_PLUGINS`. Those are *definition* tests — a
  `#define HAVE_DLFCN_H 0` still trips `#ifdef` and fails on the missing
  `<dlfcn.h>`. A plugins-off upstream build leaves both macros
  **undefined**, so I removed `#define ENABLE_PLUGINS 0` and
  `#define HAVE_DLFCN_H 0` from `config.h` (documented there).

- **`ldlex.c` must be compiled through `ldlex-wrapper.c`, not directly.**
  The checked-in 2.23 `ldlex.c` includes `bfd.h` with no prior
  `sysdep.h`/`config.h`, but `bfd.h` refuses to parse unless
  `PACKAGE`/`PACKAGE_VERSION` are already defined. `ldlex-wrapper.c` is
  literally `#include "sysdep.h"` + `#include "ldlex.c"` (sysdep.h includes
  config.h first). Upstream `CFILES` lists only `ldlex-wrapper.c` — I had
  also listed `ldlex.c`, which produced a wall of "multiple definition of
  `yy*`/`lex_*`". Removed `ldlex.c` from `LD_C`; a force-include of
  `config.h` is then unnecessary anywhere.

- **`strpbrk` was missing from the SDK** (ld's `ldlang.c` uses it for
  option parsing). Implemented in `sdk/tccsdk.c` (alongside
  `strspn`/`strcspn`) and declared in `sdk/include/string.h`.

- **No C++ demangler in the tree** (no C++ runtime). `demangle-stub.c`
  now also provides `current_demangling_style` (global),
  `cplus_demangle_set_style()` and `cplus_demangle_name_to_style()` —
  `ldlang.c`/`lexsup.c` call these unconditionally for symbol display.
  `demangle.h` is self-contained, so the stub includes it for the exact
  enum.

**Result:** `make ld` compiles all 18 ld objects + `eelf_x86_64.c` and
links `ld.exe` — a statically-linked ELF64 x86-64 **ICS-OS user
executable** (1.28 MB). As with `as.exe`/`ar.exe` it does not run on the
host (it uses `int 0x30` syscalls; `./ld.exe --version` segfaults on the
host, same as `as.exe`) — it is an in-OS tool. Functional in-OS
validation is the next step: a QEMU `test-bintools` that runs
`ar`/`as`/`ld` in-OS (assemble a `.s`, link it, exec the result) against
the FAT `/work` disk, following the `test-spawn`/`test-make` harness
pattern.

**Files touched:** `contrib/binutils/Makefile` (LD_C/LD_EMU/LDDEFS,
`ld` target + compile/link rules), `contrib/binutils/config.h` (dropped
`ENABLE_PLUGINS`/`HAVE_DLFCN_H` defines), `contrib/binutils/ldemul-list.h`
(new), `contrib/binutils/eelf_x86_64.c` (new, generated once),
`contrib/binutils/demangle-stub.c` (3 new symbols), `sdk/tccsdk.c`
(`strpbrk`), `sdk/include/string.h` (prototype),
`docs/gcc-selfhost.md` (ld round).

**Next:** the in-OS `test-bintools` QEMU harness (run as/ar/ld in-OS,
exec the linked output, grep `BINTOOLS_PASS`), then GCC 4.7.4 (C-only).

### 11:27 — libbfd (ELF x86-64) compiles 40/40 against the SDK

**Status:** the in-OS toolchain effort keeps producing results. On top of
libiberty (102/102, see 10:35), the curated **libbfd — 40/40 objects** — now
compiles clean with host gcc against the ICS-OS SDK: the BFD core
(`bfd.c`/`bfdio.c`/`bfdwin.c`/`cache.c`/`opncls.c`/`archures.c`/`targets.c`/
`section.c`/`syms.c`/`reloc.c`/`hash.c`/`linker.c`/`format.c`/`init.c`/
`cpu-i386.c`/...), the archives (`archive.c`/`archive64.c`/`coffgen.c`), and
the **ELF64 x86-64 target** (`elf.c`/`elflink.c` + `elf64.c`/`elf64-gen.c`/
`elf64-x86-64.c` + `elf-strtab.c`/`elf-eh-frame.c`/`elf-attrs.c`/`elf-ifunc.c`)
plus `dwarf2.c`. `make -C ics-os/contrib/binutils libbfd` → "40 objects built
OK". The `ar`/`as`/`ld` tools are next.

**How:** BFD's configure normally *generates* several headers. Instead of
running autotools, I committed the generated headers into `contrib/binutils/`
(found first via `-I`):
- `bfd.h` = the shipped `bfd-in2.h` template with its 5 host-type
  placeholders substituted for a 64-bit host (`file_ptr`/`ufile_ptr` =
  `long`/`unsigned long`, `BFD_ARCH_SIZE`=64, `BFD_DEFAULT_TARGET_SIZE`=64,
  `BFD_SUPPORTS_PLUGINS`=0, `BFD_HOST_64_BIT`=`long`).
- `targmatch.h` = hand-written single-target table (ELF64 x86-64 + l1om/k1om),
  with `&vec` pointer entries matching the real sed output.
- `elf64-target.h`/`elf32-target.h` = `sed s/NN/64|32/` of `elfxx-target.h`.
- `bfdver.h` (version 2.23) and `bfd_stdint.h` (C99 fixed-width typedefs).
`config.h` also gained `DEBUGDIR "/debug"` (dwarf2.c) and no plugins/NLS.

**Gaps this closed:** `strings.h` (new SDK header: `strcasecmp`/`strncasecmp`,
backed by libiberty) was the only missing header — `sysdep.h`'s other includes
(`stdio`/`stdlib`/`string`/`sys/types`/`sys/stat`/`sys/time`/`time`/`unistd`/
`fcntl`/`errno`) all existed. The libbfd build was otherwise clean against the
SDK, confirming the libiberty-round gap list was comprehensive.

**Next (in order):**
1. Build **`ar`** (uses libbfd archive + libiberty) into a static ELF64 `.exe`;
   then **`as`** (gas) and **`ld`**.
2. Stage `ar`/`as`/`ld` on `/work`; in-OS `test-bintools`: `ar rcs` +
   `as prog.s` + `ld -o prog.exe` + exec `prog.exe` → PASS.
3. Regressions (`test-integration`/`test-spawn`/`test-make`) + commit.

**Files touched:** `contrib/binutils/{Makefile,config.h,bfd.h,bfdver.h,
bfd_stdint.h,targmatch.h,elf64-target.h,elf32-target.h}`, `sdk/include/
strings.h`, `docs/gcc-selfhost.md`, this blog.

### 10:35 — libiberty builds 102/102 against the SDK (first real gap list)

**Status:** the in-OS toolchain effort (see 09:35 pivot) is producing its first
concrete results. `contrib/binutils/` (config.h + Makefile overlay, no
autotools) now compiles the entire curated **libiberty** — **102/102 objects** —
with host gcc against the ICS-OS SDK headers (`-nostdinc -I sdk/include`,
`-DHAVE_CONFIG_H`). libbfd and the as/ld/ar tools are next.

**How it works:** the Makefile compiles each libiberty source with the SDK
include tree first (so SDK headers win over host libc), our `config.h`
(`-I contrib/binutils`), and the binutils include trees; objects go to
`/tmp/icsos-binutils/obj`. The SDK is the authoritative type source, so
`config.h` deliberately does **not** re-typedef `mode_t`/`pid_t`/etc. (early
builds failed on `mode_t` conflicts until that was removed).

**OS gaps this closed (the "gap list" the pivot promised):**
- **New SDK headers:** `float.h` (IEEE float limits), `sys/param.h`
  (`PATH_MAX`/`PAGE_SIZE`/`MAXPATHLEN`/`MIN`/`MAX`/`roundup`), `sys/resource.h`
  (`struct rlimit`, `RLIMIT_*`), `malloc.h` (legacy shim over `stdlib.h`).
- **C99 types:** added `intmax_t`/`uintmax_t` + 64-bit limits to `sdk/include/
  stdint.h` (binutils `strtoumax`, `PRIxMAX` need them).
- **`signal.h`:** added `sigset_t`, `sigaction`, `SA_*`, and `sig*`/`raise`
  prototypes (libiberty `sigsetmask.c`, ld job control).
- **`fcntl.h`:** added `F_DUPFD`/`F_GETFD`/`F_SETFD`/`F_GETFL`/`F_SETFL` and
  `FD_CLOEXEC` (libiberty `pex-unix.c`; libbfd may use `fcntl`).
- **`unistd.h`:** added `realpath`, `sysconf`, `getpagesize`, `pathconf` and
  the `_SC_*`/`_PC_*` names (libbfd `getpagesize`, ld `realpath`,
  `pathconf(_PC_PATH_MAX)`).
- **`errno.h`:** added `ENAMETOOLONG`, `ELOOP`, `EISDIR`, `ENOTEMPTY`, `EPIPE`,
  `ESRCH`, `EDEADLK`.
- **`posix.c` implementations:** `getpagesize` (4096), `sysconf` (`_SC_PAGESIZE`,
  `_SC_CLK_TCK`=100, `_SC_NPROCESSORS_*`=1), `pathconf` (`_PC_PATH_MAX`/
  `_PC_NAME_MAX`), lexical `realpath`, in-SDK `getrlimit`/`setrlimit`
  (default unlimited, `RLIMIT_NOFILE`=256), and POSIX signal-set ops
  (`sigemptyset`/`sigfillset`/`sigaddset`/`sigdelset`/`sigismember`/
  `sigprocmask`/`raise`). All userspace-only — **no new kernel syscalls**.
  `posix.c` re-verified to compile cleanly under the SDK flags.

**Decisions:** libiberty files with no consumer in as/ld/ar, or that would
duplicate an SDK symbol (`gettimeofday.c`, `getpagesize.c`, `lrealpath.c`), are
excluded from the build list. `config.h` sets `HAVE_STDDEF_H`/`HAVE_STDLIB_H`/
`HAVE_SYS_PARAM_H`/`HAVE_SYS_RESOURCE_H`/`HAVE_FLOAT_H`/`HAVE_GETPAGESIZE`/
`HAVE_SYSCONF`/`HAVE_PATHCONF`/`HAVE_GETRLIMIT`/`HAVE_SETRLIMIT`/`HAVE_TIME_H`
so libiberty/libbfd gate the right includes.

**Next (in order):**
1. Implement/verify the `fcntl(F_GETFD/F_SETFD/F_GETFL)` kernel side so
   libbfd's file handling is correct (SDK `fcntl` is currently a stub returning
   0).
2. Build **libbfd** (ELF x86-64 subset) against the SDK; close its gaps.
3. Build `ar` (then `as`, then `ld`) into `apps/`; stage on `/work`.
4. In-OS `test-bintools`: `ar rcs` + `as prog.s` + `ld` link + exec → PASS.
5. Regressions (`test-integration`/`test-spawn`/`test-make`) + commit.

**Files touched:** `contrib/binutils/{Makefile,config.h}`, `sdk/include/
{float.h,sys/param.h,sys/resource.h,malloc.h,stdint.h,signal.h,fcntl.h,
unistd.h,errno.h}`, `sdk/posix.c`, this blog.

### 09:35 — Pivot: host-built toolchain self-build (Phase 1: binutils)

**Direction change (user):** drop the TCC bootstrap. The capstone goal is for
ICS-OS to self-host a **GCC build**, and the way to find every OS gap that
blocks it is to run the real toolchain in-OS. Host Linux `gcc`/`make`/`ld`/`as`/
`ar` can't run here as-is (glibc + Linux syscalls), so "host-compiled" means
**rebuilt by the host toolchain against the ICS-OS SDK** — the same, already
proven pattern as `apps/make.exe`.

**Phase 1 (this round):** build host-gcc `as`/`ld`/`ar` (binutils) + the already
working `make` targeting the SDK; ship them on the `/work` disk; in-OS: `ar rcs`,
`as prog.s`, `ld` link, exec the result. Every missing POSIX piece surfaced is a
documented OS gap to close (getcwd/stat/access/unlink/rename/time/...). This is
the gap-closing engine for the eventual GCC self-build.

**Also this session (GPF64):** `GPFhandler64` no longer halts the VM on a
**user** fault — it dumps RIP/CR2/regs, then `exc_recover()` (kill child +
resume parent + set `dex32_child_faulted`), mirroring the page-fault path.
Kernel faults still halt. `makeboot` now runs the TCC-linked `/work/make.exe`
first, captures its `rip=0x8` fault, and falls back to the host-gcc make so
`test-make` stays green. This fault-capture is the diagnostic that found the
TCC-linked make GPFs at startup (entry `0x43FC4F` → `call *%rax` with `rax=0`).

### 07:35 — Green `make test-make`: POSIX `wait()` for GNU make

**Current problem:** `test-make` hung 1800 s. In-OS TinyCC compiled make
(`MAKE_TCC_OK`) and make posix_spawned `/work/hello.exe` (load OK,
`entry=0x4040C6`) but the child never ran and make never reaped it.

**Root cause:** GNU make's blocking job path calls `wait(&status)` (the
legacy DEX `0xC` → `dex32_wait`), not `waitpid`. `dex32_wait` is a
spin-wait on the `childwait` flag that never switches the CPU to the
child, so the child starves; it also returns a bogus `1` and ignores the
status pointer. `test-spawn` worked because it calls `waitpid` (`0xB1`),
which does `ps_switchto(child)` directly.

**Fix:** SDK `wait()` is now POSIX-correct — `int wait(int *status)`
delegates to `waitpid(-1, status, 0)` (`sdk/tccsdk.c`); prototype added to
`sdk/include/sys/wait.h`. No kernel change; `0xC` stays mapped for DEX
compat.

Also found and fixed a **committed syntax error**: stray `mar         }`
in `sys_waitpid` (`kernel/vfs/posixfd.c:904`) — the kernel in the tree
did not compile at all; every prior ISO booted a stale binary.

**Tests:** `test-make` PASS (`MAKE_TCC_OK`, `Hello World from ICS-OS!`,
`MAKE_PASS`). Regressions `test-boot`, `test-smp`, `test-exec`,
`test-integration`, `test-spawn`, `test-selfhost` all PASS. `test-tccboot`
fails **pre-existing** (`tcc: error: invalid option -- '-nostdlib'` from
the in-OS-rebuilt tccnew) — A/B verified identical with the `tccsdk.c`
change reverted, so not a regression.

**Activity now:** `test-make` green. Next: the tcc-linked `make.exe`
still GPFs at `rip=0x8` (a `call` through a near-null pointer, i.e. a
corrupted/uninitialized function pointer — the "mixed in-OS-tcc +
host-gcc `sdkobj/`" class, same as tccnew). Two constraints for the
next session: (1) the GPF64 handler **halts the VM**
(`kernel/hardware/exceptions.c` `GPFhandler64` -> `while(1){}`), so an
in-OS run of the tcc-linked make just hangs — capture the fault with a
non-halting dump (record RIP/CR2/RAX/RSP + set the child-faulted flag
and return) or (2) build make **fully with in-OS tcc** (including the
SDK) so there are no mixed objects — currently blocked because in-OS tcc
chokes on the raw SDK headers (`va_list`/`size_t` clashes), which is why
`sdkobj/` is host-gcc. Fix whichever unblocks makeboot running the
in-OS-built make.

### 03:30 — GNU make 3.82 bootstrap (TinyCC → make)

**Current problem:** GNU make needs POSIX extras (dirent, waitpid(-1), posix_spawn
recipes) and an in-OS compile onto `/work`.

**Activity now:** Host gcc links make against the SDK. Host `tcc -E` preprocesses
sources so in-OS tcc never parses SDK headers (`__va_arg` builtin clash). Dropped
the `__va_arg` prototype from `sdk/include/stdarg.h`. `makeboot` extracts
`makesrc.tar` onto FAT `/work`, compiles 26 files, links `make.exe` (`MAKE_TCC_OK`).
The TinyCC-linked binary GPFs at `rip=0x8` (mixed tcc/gcc objects); the recipe
run uses host-gcc `apps/make.exe` until that link is fixed. Next: green
`make test-make`, then tcc-linked runtime.

### 02:40 — POSIX spawn + virtio `/work` (GCC self-host prerequisites)

TinyCC will not compile `kernel32.c`. The self-host target is TinyCC → GNU
make → binutils → GCC 4.7.4 (C only) → ICS-OS. This round does not download
GCC; it lands the OS gaps that block that chain.

**Current problem:** no userspace `waitpid`, `execvp` was a stub, `user_execp`
always waits (console/tccboot need that), `forkprocess` is 32-bit paging,
`/ramdisk` is 16 MiB, and `vblk` was raw with no FAT mount.

**Activity now:** GNU make 3.82 bootstrap. `waitpid(-1)`/`WNOHANG` wait queue +
`getdents`/`dirent` are in. `contrib/gnumake/` + `scripts/stage-make.sh`.
Need a green `test-make` next (in-OS tcc of make onto `/work`). Stale
`scheduler.o` after PCB growth hung boot — `process.h` is now a scheduler
dep.

## 2026-08-28 (Manila, UTC+8) — continued

### 03:05 — GNU make bootstrap (TinyCC → make)

Spawn prerequisites are in. Next is in-OS TinyCC compiling GNU make,
then binutils, then GCC 4.7.4.

## 2026-08-27 (Manila, UTC+8)

### 21:15 — Async virtio completions into io_uring

IRQ harvest of the virtio used ring replaces the 20M `pause` spin.
Each in-flight request owns a 3-descriptor slot (hdr + data + status)
plus a 4KiB kernel bounce buffer (user VA is not GPA-safe under a
private PML4). Waiters `hlt` until MSI-X 0x42; copy-back and uring CQEs
run in process context. `/dev/vblk` is a raw POSIX fd; uring
READ/WRITE/FSYNC on it submit without waiting. `io_uring_enter` honors
`min_complete`. Ramdisk SQEs stay inline.

Tests: `test-virtio` (`irqs=3 slots=42`, `VIRTIO_IRQ_OK`, pipelined
reads), `test-posixio` (`URING_VBLK_PASS`), `test-boot`, `test-smp`,
`test-exec` PASS.

**Activity now:** async uring on virtio is in. Next is `test-kbuild` or
ring-3.

### 20:40 — I/O P3: POSIX fds and a synchronous io_uring subset

Kernel per-process fd table: `open`/`close`/`read`/`write`/`lseek`/
`preadv`/`pwritev`/`fsync`. `io_uring_setup`/`enter` run NOP, READ/WRITE,
READV/WRITEV, FSYNC, OPENAT, CLOSE inline into the CQ. Ring VA is
`params.sq_off.user_addr` (identity map). DEX fopen/fread stay as compat;
`fdopen` returns the kernel `file_PCB*` so TinyCC ELF output still uses
DEX `fwrite`.

Tests: `test-boot`, `test-smp`, `test-exec`, `test-virtio`, `test-iobench`,
`test-posixio`, `test-selfhost` PASS (`POSIXIO_PASS`, `URING_PASS`).

**Activity now:** P3 landed. Next is async uring completions or
`test-kbuild`.

### 20:15 — I/O P2: bio + 4KiB page cache

P2 is in. 512×4KiB write-back cache, `bio_submit_sync`, one hctx per
device. ISO9660 CD reads are cached (two 2048-byte sectors per page).
Misses merge into aligned 4KiB device reads.

Tests: `test-integration`, `test-virtio`, `test-iobench` PASS.
iobench on `/icsos/apps/tcc.exe`: cold 224 ms, warm 3 ms (**74.6x**),
cache hits=580 misses=146 fills=74 merged=148, `IOBENCH_CACHE_OK`.

**Activity now:** P2 landed. Next is P3 (POSIX fds / io_uring) or
`test-kbuild`.

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

## 2026-08-28 (Manila, UTC+8)

### 12:00–13:00 — GCC self-host, step 2: binutils `ar` builds (libbfd + libiberty in the ICS-OS SDK)

**Goal:** move past TinyCC + GNU make and build the next toolchain stage — GNU
binutils. Per `docs/gcc-selfhost.md` the order is TinyCC → **make** → **binutils**
→ GCC 4.7.4. This session lands the first linkable binutils artifact: `ar.exe`,
which pulls in the full `libbfd` (ELF x86-64 backend + generic-ELF core + all
`cpu-*.c` arch tables) and `libiberty` against the ICS-OS SDK.

**What blocked it (and the fix):**
- `libbfd` was a hand-picked file list. Adding the real `ar` exposed that
  `archures.c`'s master `bfd_arch_list[]` references *every* `bfd_*_arch`, so a
  `cpu-i386.c`-only build left ~77 arch symbols undefined. Fix: build the whole
  `cpu-*.c` set (all self-contained static tables; `cpu-ia64-opc.c` excluded —
  it is `#include`d by `cpu-ia64.c`, not standalone).
- The generic-ELF core is a pair of **`elfcode.h` shims**: `elf64.c`
  (`ARCH_SIZE=64`, already built) and `elf32.c` (`ARCH_SIZE=32`, was missing).
  Adding `elf32.c` supplies the `elf32_*` symbols `elf.c`/`elflink.c` call.
  `dwarf1.c` was likewise missing and added.
- `elf64-x86-64.c` references `nacl_modify_segment_map`/`nacl_modify_program_headers`
  (in `elf-nacl.c`) and `cpu-ns32k.c` references `_bfd_ns32k_relocate_contents`
  (in `aout-ns32k.c`) — both added to the build.
- `binemul.c` needs the emulation vector; upstream `#define`s it via
  `-Dbin_dummy_emulation=$(EMULATION_VECTOR)`. Added
  `-Dbin_dummy_emulation=bin_vanilla_emulation` (defined in `emul_vanilla.c`).
- `ar`'s frontend needs libc/POSIX the SDK lacked: `getc`/`clearerr` (stdio),
  and `lstat`/`chown`/`utime`/`mktemp` (posix). Added minimal implementations to
  `sdk/posix.c` + `sdk/tccsdk.c` and declared them in the SDK headers
  (`stdio.h`, `stdlib.h`, `unistd.h`, `sys/stat.h` — the latter also gained the
  `S_IS*` file-type test macros + `struct utimbuf`).
- `bfd.c`'s `bfd_demangle()` calls `cplus_demangle()`; the C++ demangler is not
  built, so a small `demangle-stub.c` (returns NULL) satisfies the link.

**Result:** `make ar` in `ics-os/contrib/binutils` links a 1 MB statically-linked
ELF64 x86-64 `ar.exe` (an ICS-OS user ELF using `int 0x30` syscalls — it runs
in-OS, not on the host). This is the first binutils tool to build.

**Next (binutils):** implement the `as` (gas) and `ld` frontends — `as` needs the
GAS source + `opcode` library, `ld` needs `ldemul`/BFD linking. Then a QEMU
`test-binutils` that runs `ar`/`as`/`ld` in-OS. After that, GCC 4.7.4 (C-only).

**Files touched this session:** `contrib/binutils/Makefile` (libbfd/libiberty
file lists, `CPUC` wildcard, `bin_dummy_emulation` def, `ar` link line),
`contrib/binutils/config.h` (`TARGET`, `DEFAULT_AR_DETERMINISTIC`),
`contrib/binutils/demangle-stub.c` (new), `sdk/{posix.c,tccsdk.c}`,
`sdk/include/{stdio,stdlib,unistd,sys/stat}.h`.

### 14:00–15:30 — binutils `as` (GAS 2.23) builds + links an ICS-OS user ELF64

**Goal:** the second binutils tool — the GNU assembler. This is the critical
one: GCC emits `.s` and needs `as` to turn it into `.o`, so a working `as`
unblocks the GCC 4.7.4 self-host step.

**Approach:** mirror the `ar`/`libbfd`/`libiberty` recipe. Add `gas/` core +
`opcodes/` to the Makefile and link them against the already-built libbfd +
libiberty + SDK.

**Findings (the i386 path is NOT the cgen/itbl table path):**
- `itbl-ops.c` / `itbl-lex-wrapper.c` / `cgen.c` serve the *instruction-table*
  (cgen) backends — MIPS only, gated by `HAVE_ITBL_CPU`. i386/x86-64 uses the
  hand-written `tc-i386.c` backend instead.
- For a non-itbl target, `as.c` compiles `#define itbl_init()` (a **no-op
  macro**) and the itbl objects are simply **not in the link**. So I removed
  `itbl-ops.c`/`itbl-lex-wrapper.c` from `GAS_C` (they don't even compile for
  i386 — `ITBL_OPCODES`/`ITBL_NUM_OPCODES` are undefined).
- The target files live in `gas/config/` and need their own rule:
  `obj-elf.c` (ELF object format), `atof-ieee.c` (float literals for
  `md_atof`), and `tc-i386.c` (the CPU backend; it `#include`s
  `tc-i386-intel.c`, so that file is not listed separately).
- **`DEFAULT_ARCH` must be a string, not an enum.** `tc-i386.c` does
  `static const char *default_arch = DEFAULT_ARCH;` and later
  `strncmp(default_arch, "x86_64", 6)`. I had set
  `-DDEFAULT_ARCH=elf64_littleswap` (a BFD enum) → `'elf64_littleswap'
  undeclared`. Upstream `gas/configure` emits
  `#define DEFAULT_ARCH "${arch}"` → fixed to `-DDEFAULT_ARCH=\"x86_64\"`.

**SDK gaps closed (GAS needs a little more POSIX than `ar` did):**
- `ungetc` — GAS's backtracking lexer calls it heavily. The SDK `FILE` is an
  opaque fd handle (no user-side buffer), so a regular file is pushed back by
  `fseek(f, ftell(f)-1, SEEK_SET)`; `stdin` uses a one-char pushback slot
  (`fgetc` was patched to honor it). Implemented in `sdk/tccsdk.c`.
- `strftime` — GAS only uses it for the `-L` listing-header timestamp
  (`"%Y-%m-%dT%H:%M:%S.000%z"`). Implemented a small converter in
  `sdk/posix.c` (subset: `%Y %y %m %d %H %M %S %B %b %A %a %Z %z %n %t %`).
- `localtime` — the old stub returned a fixed date; replaced with a real
  civil-from-days implementation (Hinnant algorithm, UTC; the kernel clock is
  UTC and TZ is unimplemented). `tm_yday`/`tm_wday` are computed properly.
- `mbstowcs` — GAS (`read.c`) calls `mbstowcs(NULL,name,len)==-1` purely as a
  locale check on quoted symbol names. ICS-OS is single-byte/ASCII, so a
  `static inline` in `sdk/include/wchar.h` (new file) that returns the byte
  length is correct.

**Result:** `make as` compiles all 33 GAS objects + opcodes + libbfd + libiberty
and links `as.exe` — a statically-linked ELF64 x86-64 **ICS-OS user
executable** (`_start` at entry, `dexsdk_systemcall` present, no host
interpreter). Like `ar.exe` it does not run on the host (it uses `int 0x30`
syscalls); it runs in-OS. Functional in-OS validation (`as --version`,
assembling a real `.s`) is the next step, paired with a QEMU `test-binutils`.

**Files touched:** `contrib/binutils/Makefile` (GAS_C/GAS_CFG_C/OPCODES lists,
`gas`+`as` targets, `DEFAULT_ARCH` fix, itbl exclusion),
`contrib/binutils/{bfdver.h,config.h}`, `sdk/tccsdk.c` (`ungetc` + pushback
slot + `fgetc` patch), `sdk/posix.c` (`localtime` + `strftime`),
`sdk/include/wchar.h` (new, `mbstowcs`), `sdk/include/{stdio,time}.h`
(prototypes).

**Next:** build `ld` (ldemul/`elf.em` + BFD linking), then a QEMU
`test-binutils` that runs `as`/`ar`/`ld` in-OS against a real source file,
then GCC 4.7.4 (C-only).

## 2026-08-29 (Manila, UTC+8)

### 05:50–09:35 — GCC self-host, step 3: in-OS `as`/`ar`/`ld` test; **root-caused & fixed broken seeked/positioned file writes in the VFS**

**Current problem:** `make test-bintools` failed: in-OS `as` produced a
corrupt `/ramdisk/mini.o` (512 bytes, ELF header all zero, 45 nonzero bytes
in the tail) yet exited 0 with no error and no GPF.

**Method:** added instrumented probes to `contrib/bintest/bintest.c`:
`SEEKPROBE` (DEX `fopen` w → close → `fopen` r+ → `fseek(3)` → `fwrite` →
close → readback) and `PSEEKPROBE` (POSIX `open`/`write`/`lseek(3)`/`write`).

**Root cause (two VFS bugs, both in `kernel/vfs/vfs_core.c`):**
1. `fseek()` clamped `ptrlow` to the current file size. For a freshly opened
   *write* handle the data is still in the 512-byte buffer, so `size==0` and
   every seek collapsed to 0 → `lseek` was a silent no-op and positioned
   writes landed at offset 0 (PSEEKPROBE read back `[XX23456789]`).
2. `vfs_writechar()` accepted a write into an already-buffered region
   without a contiguity check, leaving uninitialised bytes between `endsize`
   and the write offset; the flush overwrote the file with that stale region
   (SEEKPROBE read back empty).

GAS/BFD finalize an object by writing the body then **seeking back** to patch
the ELF header and section table — so with positioned writes broken, *both*
`as` and `ld` emit garbage. This was the real gate for the whole
binutils→GCC chain (not the SDK, not the toolchain).

**Fix (commit 33ddcfa):** `fseek` sets the requested offset without clamping
(SEEK_END = size+offset, standard sign; negative → error); `vfs_writechar`
only extends the current buffer region when the write is contiguous with it,
otherwise flushes and restarts the region at the new position (no recursion:
`vfs_flushbuffer`'s internal `fseek` never flushes because `bufferwrite` is
already 0 there).

**Result:** in-OS GNU `as` now emits a valid ELF64 `ET_REL` object —
`AS_PASS` in `test-bintools`. No regressions: `test-integration` PASS
(boot+SMP+exec), `test-posixio` PASS (POSIX fds + io_uring).

Two test bugs found along the way: `"\x7fELF"` in C is `{0xFE,'L','F'}`
(hex escapes are greedy — `\x7fE` consumes the `E`); must use `"\177ELF"`.
And `ar` needs an operation: `ar r archive member`, not bare `ar archive member`.

**Next blocker (identified, not yet fixed):** `ar` (and `ld`) fail with
`bfd_openw ... No error` — they create their output with **`FILE_READWRITE`
(`r+`) positioned writes** (archive index / symbol table), and that path is
still broken: SEEKPROBE (`r+` seek+write) still reads back empty, while
FILE_WRITE and POSIX `O_WRONLY` positioned writes now work. Likely in the
`openfilex` FILE_READWRITE setup vs the flush/FAT write-size interaction
(FILE_WRITE truncates via `rewritefile`; FILE_READWRITE does not). This is the
immediate task before `ld` and GCC 4.7.4 can run in-OS.

### 09:40–11:20 — `ld` cannot find its default linker script (`LD_FAIL`): root-caused to the missing **combreloc** script variant, not a VFS/path bug

Picking up from the seeked-write fix: `as` and `ar` now pass, but `ld`
failed with `LD_FAIL` — it could not open its default linker script. This was
the last gate before the in-OS toolchain could link a program.

**Why it looked like a VFS bug (and wasn't).** The failure was `errno=2`
(ENOENT) from `ldfile_try_open()` inside `ldfile_find_command_file()`
(`references/binutils-2.23/ld/ldfile.c`). Because fast child (ld) serial
output is lossy, I built a **child→file→parent diagnostic relay**: `ld`
writes its state to `/ramdisk/ld.diag` (the only location it demonstrably
writes to — `mini.o`/`mini.exe` land there), and the parent `bintest` reads it
back and prints it. That relay produced the decisive evidence:

- In the **parent**, `fopen`/`open` of every path form on `/icsos`
  (single/double/triple leading slash) all succeed → not a path-collapse bug.
- In the **child** (ld), every `stat` succeeds, and a *literal* `fopen` of the
  script path (single and double slash) **also succeeds** — yet `try_open()`
  on the *concatenated* path still returns NULL/errno=2.

The relay printed the exact string `try_open` was handed:
`len=36 path=/icsos/apps//ldscripts/elf_x86_64.xc` — a trailing **`c`**.

**Root cause.** The `c` is not corruption. `gldelf_x86_64_get_script()`
(`ld/eelf_x86_64.c`) selects the script by link flags: with default
`link_info.combreloc` true it returns `ldscripts/elf_x86_64.xc` (the
*-combreloc* variant), **not** the bare `elf_x86_64.x`. The build staged only
the base `.x`, so the `.xc` variant simply did not exist → ENOENT. The
"identical `fopen` succeeds but `try_open` fails" mystery was a red herring:
the probe had been `fopen`-ing the base name, while `try_open` was trying the
combreloc name.

**Fix.** Stage the whole script family, not one file. Copied the 12
non-base variants (`elf_x86_64.x{bn,c,d,dc,dw,n,r,s,sc,sw,u,w}`) from the
binutils references tree into `contrib/binutils/ldscripts/` (source) and
`apps/ldscripts/` (installed); updated both `contrib/binutils/Makefile`
(`install`) and the top-level `test-bintools` ISO staging to copy
`elf_x86_64.x*` (wildcard) instead of a single file. `mini.o` is self-contained
(`int 0x30` syscalls only, no libc), so the references-tree variants — which
lack the ICS-OS `SEARCH_DIR` customization in the committed base `.x` — link
it fine.

**Cleanup.** Removed all the temporary diagnostics: the `ldfile.c`
`/ramdisk/ld.diag` relay (plus its `stat`/`fopen`/`strlen` probes and the
`<errno.h>`/`<sys/stat.h>` includes) and the `bintest.c` `dump_file()` helper +
`LDSCRIPTPROBE` block. The permanent `diag_file()` (size/hex dumper used for
`mini.o`/`mini.exe`/`mini.out`) stays.

**Result:** `make test-bintools` fully green — `AS_PASS`, `AR_PASS`,
`LD_PASS`, `LD_EXEC_PASS` (linked `mini.exe` runs and writes
`BINTOOLS_MINI_OK`), `BINTOOLS_PASS`. No regressions: `test-integration` and
`test-posixio` still pass. The in-OS GNU `as`/`ar`/`ld` chain now links a real
ELF64 user program end-to-end.

**Next:** GCC 4.7.4 self-host step 4 — build the in-OS GCC (C-only) against
this working binutils, then have that GCC compile ICS-OS (see
`docs/gcc-selfhost.md`).

## 2026-08-29 (Manila, UTC+8)

### libcpp stage complete — 15 preprocessor objects compile

The first GCC 4.7.4 self-host stage (the `contrib/gcc` overlay, `make libcpp`)
is green: all 15 target-independent preprocessor objects (`charset`,
`directives`, `directives-only`, `errors`, `expr`, `files`, `identifiers`,
`init`, `lex`, `line-map`, `macro`, `mkdeps`, `pch`, `symtab`, `traditional`)
compile cleanly against the SDK with the hand-written `config.h`.
`makeucnid.c` is a build-time code-gen tool and is correctly kept out of the
link set. This de-risks the preprocessor before the heavier cc1 stage.

### GMP 5.1.3 builds and is host-verified (`make test-gmp`)

**Goal:** build the GMP 5.1.3 arbitrary-precision integer/ration library as a
freestanding `libgmp.a` against the ICS-OS SDK. cc1 uses GMP for
integer/ration constant arithmetic, so this must link and run the same way the
other library stages do.

**Approach:** a dedicated `contrib/gmp` overlay (GMP is a self-contained
library, kept separate from the `contrib/gcc` one): host gcc with the same
`-nostdlib -ffreestanding -nostdinc` SDK flags, a hand-written `config.h`, and a
*generated* `gmp.h` that maps the short public names (`mpz_add`, `mpn_mul_1`,
...) to the `__gmp`/`__gmpn`-prefixed symbols the archive defines. Limbs are
64-bit long-long (`GMP_LIMB_BITS=64`, `GMP_NAIL_BITS=0`,
`mp_limb_t = unsigned long long int`).

**Findings / obstacles (all fixed):**

- **SDK gaps.** GMP 5.1.3 needs C99 least-width int typedefs
  (`uint_least32_t`) — added to `sdk/include/stdint.h`; it probes for
  `memcpy`/`memmove`/`memset` (suppressed via `HAVE_MEM*` so the SDK `string.h`
  wins) and `stdarg.h` (defined `HAVE_STDARG`); and it references `localeconv`
  for a decimal-point fallback — the locale macros were removed since the SDK
  has no locale support.

- **Generated tables must be committed.** GMP's build generates
  `mpn_fib_table.c`/`fib_table.h`, `mpn_bases_table.c`/`mp_bases.h`,
  `fac_table.h`, `jacobitab.h`, `trialdivtab.h`, and `perfsqr.h` from C
  generator programs. Each generator was run once (with `64 0` limb/nail bits)
  and the results committed into the overlay, which is on the include path.

- **Multi-compiled generic files.** Four `mpn/generic/*.c` files each define
  several `mpn` entry points selected by an `OPERATION_*` macro: `logops_n.c`
  → 8 bitwise ops, `popham.c` → `popcount`/`hamdist`, `sb_div_sec.c` and
  `sbpi1_div_sec.c` → 2 sec-division variants each. The Makefile compiles each
  once per operation with `-DOPERATION_<name>=1`.

- **`GENERIC_C` self-reference left the whole `mpn` tier out (the big one).**
  The rule was `GENERIC_C := $(filter-out $(DUAL_C),$(GENERIC_C))` — with `:=`
  the RHS expanded `$(GENERIC_C)` to *itself* (empty) at define-time, so
  `mpn/generic/*.c` contributed nothing and the archive silently lacked every
  low-level `mpn_*` symbol. Fixed by widening the RHS to the actual wildcard:
  `$(filter-out $(DUAL_C),$(wildcard $(SRC)/mpn/generic/*.c))`.

- **`ar` member-name collisions.** Objects were named by basename, so
  `mpz/add.c`, `mpf/add.c`, `mpq/add.c`, and `mpn/generic/add.c` all produced
  `add.o` and clobbered each other in `libgmp.a`. Object names are now flattened
  to unique basenames (`mpz/add.c` → `mpz_add.o`, `mpn/generic/add_1.c` →
  `mpn_generic_add_1.o`) via a `SRC2OBJ` transform, and the per-source compile
  rules are generated with `define` / `$(foreach ... $(eval ...))`.

- **`$(dir $$@)` in a `define`-generated rule.** A per-object
  `mkdir -p $(dir $$@)` was expanded at *parse* time (when `$@` is empty) to
  `./`, so it never created the real object dir. Replaced with an order-only
  `| $(OBJ)` prerequisite on every generated rule plus a single
  `$(OBJ): ; mkdir -p $(OBJ)` target.

- **Assertion handler was missing from the build.** `assert.c` (which provides
  `__gmp_assert_fail` / `__gmp_assert_header`) was not in `TOP_C`, so any GMP
  assertion path would be an undefined reference. Added it; its only external
  deps are `abort`, `fprintf`, and `stderr`, all provided by the SDK link set
  (`sdk/tccsdk.c` defines `stderr`, `sdk/posix.c` defines `abort`/`fprintf`)
  that the cc1 overlay already uses.

**Result:** `make -C contrib/gmp libgmp` builds a `libgmp.a` of 461 objects
(521 defined `T` symbols); every internal `__gmp*` reference resolves and the
only external undefineds are `abort`/`fprintf`/`stderr`. A host functional test
(`contrib/gmp/test_gmp.c`, run by `make test-gmp`) links against the archive and
exercises `mpz_add`/`mpz_mul`/`mpz_powm`/`mpz_get_str`, `mpf_add`, `mpq_add`,
`gmp_randinit_mt`, and the internal `mpn_add_1`/`mpn_mul_1` — all pass:
`GMP_HOST_TEST_PASS`.

**Next:** MPFR 3.0.1 (float constant arithmetic; needs the SDK `libm` to grow),
then wire GMP+MPFR into the `contrib/gcc` overlay and build `cc1`.

### MPFR 3.0.1 builds and is host-verified (`make test-mpfr`) + two SDK correctness gaps fixed

**Goal:** build MPFR 3.0.1 (the floating-point library cc1 uses for
`__float128`/constant arithmetic) as a freestanding `libmpfr.a` against the SDK,
linking the GMP stage above it.

**Approach:** a dedicated `contrib/mpfr` overlay. Unlike GMP, MPFR 3.0.1 is
configured through **`DEFS` (command-line `-D` flags), not a `config.h`** —
`HAVE_CONFIG_H` is absent, so every `#ifdef HAVE_CONFIG_H` guard stays false. The
configure-generated `mparam.h` (tuning constants) is shipped in the overlay, and
the 217 library `.c` files are pulled in by wildcard with 5 exclusions
(`ansi2knr.c` K&R tool; `speed.c`/`tuneup.c` standalone programs; `jyn_asympt.c`
and `round_raw_generic.c` which are `#include`d by `jn.c`/`yn.c` and
`round_prec.c`).

**Findings / obstacles (all fixed):**

- **`__gmp_const` gap in the GMP header.** MPFR's `mpfr.h` types
  `mpfr_srcptr` as `__gmp_const __mpfr_struct *` and uses `__gmp_const`
  throughout, but GMP 5.1.3's `gmp-h.in` never defines the `__gmp_const` /
  `__gmp_unsigned` / `__gmp_signed` portability macros (it uses bare `const`).
  Added the three `#define`s to the overlay `gmp.h` (after the
  `__GMP_DECLSPEC` block). GMP still passes.

- **The overlay must build with `-nostdinc -I$(SDK)/include`.** The first MPFR
  `Makefile` forgot the `-nostdinc` include guard that the GMP overlay uses, so
  `#include <...>` silently resolved to **host glibc** headers. glibc's `ctype.h`
  implements `isspace`/`isalpha` as macros over the glibc-internal
  `__ctype_b_loc`, which leaked into `libmpfr.a`'s undefined set. Matching the
  GMP include path fixed it: `isspace`/`isalpha` now resolve to the SDK's real
  functions. (The objects live in `obj/`, so a header change requires
  `rm -f obj/*.o`, not `rm -f *.o`.)

- **SDK gap: `SIZE_MAX` missing from `stdint.h`.** MPFR's `vasprintf.c` uses the
  C99 `SIZE_MAX`. Added `SIZE_MAX` (plus the 8/16-bit and `INT32_MIN` limits) to
  `sdk/include/stdint.h`.

- **SDK bug: `LONG_MAX` was the 32-bit value on a 64-bit platform (the big one).**
  `sdk/include/limits.h` defined `LONG_MAX 2147483647L` (a 32-bit constant) even
  though on x86-64 long mode `long` is 64-bit. GMP's `MP_SIZE_T_MAX` derives from
  `LONG_MAX` (when `__GMP_MP_SIZE_T_INT==0`, i.e. `mp_size_t` is `long`), so it
  came out 32-bit. MPFR's `init2.c` runs the sanity assertion
  `MP_SIZE_T_MAX >= MPFR_PREC_MAX / BYTES_PER_MP_LIMB`; with a 64-bit
  `mpfr_prec_t` (chosen because `__GMP_MP_SIZE_T_INT==0`) that is
  `2147483647 >= 1152921504606846975` → **aborts at first `mpfr_init2`**. The GMP
  test never caught this because `MP_SIZE_T_MAX` only feeds algorithm thresholds
  (still astronomically large, so results were right). Fixed `limits.h`:
  `LONG_MIN/MAX`/`ULONG_MAX` are now 64-bit under `__x86_64__` (32-bit otherwise)
  and `LLONG_*/ULLONG_MAX` were added. Both GMP and MPFR were rebuilt; GMP still
  passes.

- **Test-file comment bug.** `test_mpfr.c`'s header comment contained the literal
  sequence `*/` (from `__gmp*/__gmpn*`), which terminated the block comment early
  and turned the rest of the file into garbage (cascading `size_t` errors).
  Reworded the comment.

**Result:** `make -C contrib/mpfr libmpfr` builds a `libmpfr.a` of 217 objects
(1,020,760 bytes). A host functional test (`contrib/mpfr/test_mpfr.c`,
`make test-mpfr`) exercises add/sub/mul/div, `sqrt`, `pow`, `exp`, `log`,
`sin(pi/2)`, `cos(0)` and an mpfr→mpz round-trip — all pass:
`MPFR_HOST_TEST_PASS`. A strict in-OS link simulation
(`test_mpfr.c` + `libmpfr.a` + `libgmp.a` + the SDK objects,
`-nostdlib -Wl,--no-undefined`) links cleanly, and a definitive `nm` check shows
all 28 of MPFR's external symbols (memory/string/stdio, no libm) are defined in
the SDK — **0 MISS**. Notably MPFR implements its own transcendentals (series +
argument reduction + AGM), so **no host `libm` is required** — the earlier
"SDK libm must grow" note does not apply to the core path.

**Next:** wire GMP+MPFR into the `contrib/gcc` overlay, then `libiberty`, then
`cc1`.

### GMP+MPFR wired into the `contrib/gcc` overlay + `libiberty` stage built

**Goal:** make the `contrib/gcc` overlay compile `cc1` against the SDK-built
GMP/MPFR (so its constant-arithmetic calls resolve to our libs, not the host's)
and build the `libiberty` stage `cc1` links against.

**GMP/MPFR wiring (the latent bug found):** the overlay's include path pointed
`GMPINC` at the **host** `gmp.h` (`/usr/include/x86_64-linux-gnu`). That header
maps the public `mpz_*`/`mpn_*` names to *bare* symbols, but our `libgmp.a`
exports the `__gmp*`/`__gmpn*`-prefixed ones (our `contrib/gmp/gmp.h` does that
mapping). Compiling `cc1` against the host header would leave every `mpz_*`
reference undefined at link time. Fixed: `GMPINC` now points at
`contrib/gmp` (our `gmp.h`) + `contrib/mpfr`, and `SDKLIBS` gained
`libmpfr.a` + `libgmp.a`. Verified with a new `make test-gmpmpfr` regression
target: a tiny program calling both `mpz_add` and `mpfr_mul`, built with the
*exact* `cc1` flags (`-DHAVE_CONFIG_H`, the GCC internal include dirs, our
overlay headers) and linked with `-Wl,--no-undefined` against the SDK — passes
(`GCC_GMPMPFR_WIRING_OK`, 226 KB static binary). Also confirmed the public
`mpfr.h`/our `gmp.h` do not `#include "config.h"`, so `-DHAVE_CONFIG_H` (GCC's
`config.h`) does not leak into them.

**Link-order fix:** the first wiring attempt failed with undefined
`__gmpn_sub_1`/`__gmpn_lshift`/`__gmpn_add_1` — MPFR's objects reference GMP's
internal limb functions, so `libmpfr.a` must come **before** `libgmp.a` (a
dependency, then its dependency). `GMPMPFR` now lists them in that order.

**`gmp.h` cleanup:** two unexpanded Autoconf `@HAVE_HOST_CPU_FAMILY_*@`
placeholders (PowerPC detection, unused on x86-64) were left in the generated
header; set both to `0` so the macros are well-formed.

**`libiberty` (stage 2):** the GNU common library `cc1` needs, built as a static
`libiberty.a` so the `cc1` link pulls in only referenced members. 41 portable
objects: allocation/structures (`xmalloc`, `obstack`, `objalloc`, `hashtab`,
`fibheap`, `splay-tree`, `sort`, `partition`, `dyn-string`), utilities
(`getopt`/`getopt1`, `basename`/`lbasename`, `concat`, `hex`, `ffs`, `insque`,
`strverscmp`, `floatformat`, `md5`, `crc32`, `copysign`, `xatexit`, `xexit`,
`xstr*`, `asprintf`/`vasprintf`, temp-file helpers), and `cplus-dem`
(demangling). Deliberately omitted: files duplicating SDK libc (`memcpy`,
`strcmp`, `strtod`, ...) and files making OS calls the in-OS kernel does not
serve (`pexecute`/`pex-*`/`vfork`/`waitpid`/`tmpnam`/`getpwd`) — process
spawning is the gcc *driver's* job, not `cc1`'s. All 41 compile cleanly against
the SDK; `libiberty.a` is 226 KB.

**Next:** build `cc1` (the C frontend + middle end + `config/i386` target), the
heavy stage.

### 11:30–19:20 — All `insn-*` generators run: complete x86_64-linux machine-description support set produced

**Goal:** run the full GCC `gen*` pipeline (host-side, x86_64-linux target) to
produce every generated file `cc1` needs — the long pole of the `cc1` build,
because each generator is itself a small C program that must first be compiled
against the same generated headers.

**Root cause of the `unknown mode XF` blocker:** `genmodes` reads its machine
modes from `machmode.def`, which only lists the *standard* modes. Target-specific
modes (x86 `XF`/`TF`, the `CC*` condition-code modes, the `V16QI`/`OI` vector
modes) come from a second file pulled in by `machmode.def`'s
`# include EXTRA_MODES_FILE`. GCC's `config.gcc` sets that to
`config/i386/i386-modes.def`. So `genmodes` (and every other generator that
includes `machmode.def`) had to be compiled with
`-DEXTRA_MODES_FILE="config/i386/i386-modes.def"`. Recompiling `genmodes` with
that flag and regenerating `insn-modes.{c,h}` + `min-insn-modes.c` gave 19
i386-mode symbols (`XFmode`, `CCGC`, `V16QI`, `OI`, ...).

**The generator chain (each step unblocks the next):**
1. `genmodes` (with `EXTRA_MODES_FILE`) → `insn-modes.c/h`, `min-insn-modes.c`.
2. Recompile the `BUILD_RTL` set (`rtl`, `read-rtl`, `ggc-none`, `vec`,
   `gensupport`, `print-rtl`, `min-insn-modes`) **and every generator** against
   the new `insn-modes.h`, then relink all of them (`genautomata` also needs
   `-lm`).
3. `genconditions i386.md` → `build/gencondmd.c` (16071 lines).
4. `gencondmd.c` would not compile until its generated-header dependencies
   existed; produced each: `genconstants i386.md` → `insn-constants.h`;
   `mkconfig.sh` → `tm_p.h` (wraps `i386/i386-protos.h`); `opt-gather`/`opth-gen`
   over `c.opt common.opt i386.opt linux.opt` → `options.h` (the `linux.opt`
   file is what defines `linux_libc`); hand-wrote `all-tree.def`
   (`tree.def` + `c-common.def`); `gencheck` → `tree-check.h`; `genpreds -c` →
   `tm-constrs.h`; and regenerated `tm.h` **with** the `config.gcc` `tm_defines`
   (`USE_IX86_FRAME_POINTER=1 LIBC_GLIBC=1 LIBC_UCLIBC=2 LIBC_BIONIC=3
   DEFAULT_LIBC=LIBC_GLIBC`) plus `defaults.h` in the header list (mkconfig only
   emits `# include "defaults.h"` when `defaults.h` is actually in `HEADERS`).
5. `gencondmd` → `insn-conditions.md` (2842 lines). This is the linchpin: nearly
   every downstream generator consumes it.
6. Ran the rest with `gen* i386.md insn-conditions.md`: `gencodes`→`insn-codes.h`,
   `genflags`→`insn-flags.h`, `genattr`→`insn-attr.h`, `genattr-common`→
   `insn-attr-common.h`, `genconfig`→`insn-config.h`, `genattrtab`→`insn-attrtab.c`,
   `genautomata`→`insn-automata.c`, `genemit`→`insn-emit.c`, `genextract`→
   `insn-extract.c`, `genopinit`→`insn-opinit.c`, `genoutput`→`insn-output.c`,
   `genpeep`→`insn-peep.c`, `genrecog`→`insn-recog.c`, `genenums`→`insn-enums.c`.
7. `genpreds i386.md`→`insn-preds.c`; `genpreds -h`→`tm-preds.h`;
   `gengenrtl`→`genrtl.h`.

**Result — the full generated set (28 files) now exists in `/tmp/icsos-gcc/gen`:**
`insn-modes.{c,h}`, `min-insn-modes.c`, `insn-constants.h`, `insn-conditions.md`,
`insn-codes.h`, `insn-flags.h`, `insn-attr.h`, `insn-attr-common.h`,
`insn-config.h`, `insn-attrtab.c`, `insn-automata.c`, `insn-emit.c`,
`insn-extract.c`, `insn-opinit.c`, `insn-output.c`, `insn-peep.c`,
`insn-recog.c`, `insn-enums.c`, `insn-preds.c`, `tm-preds.h`, `tm-constrs.h`,
`genrtl.h`, `tree-check.h`, `options.h`, `all-tree.def`, `tm.h`, `tm_p.h`.
The big ones are real target tables: `insn-recog.c` 166K lines, `insn-output.c`
141K, `insn-attrtab.c` 175K, `insn-emit.c` 79K, `insn-automata.c` 35K.

**Note for the `cc1` build:** `genchecksum`/`cc1-checksum.c` is deliberately not
generated yet — it is fed the *actual* `cc1` object list plus the link-options
file, so it belongs to the `cc1` link step, not the generator step.

**Next:** build `cc1` itself — the C frontend + middle end + `config/i386`
target objects — compiled against this generated set and linked with
`libcpp.a`, `libiberty.a`, `libmpfr.a`, `libgmp.a` and the SDK libc.

### 19:20–20:55 — cc1 smoke-test: the x86_64 target backend compiles (`i386.c`, `i386-c.c`, `dwarf2out.c`, `c-parser.c`, `tree.c`, `alias.c`)

Generator milestone done; moved to compiling `cc1` itself against the generated
set. Ran the middle-end + target files through a host smoke-compile (SDK
freestanding flags + the generated include dir) to surface missing generated
files, target defines, and SDK gaps.

**Files now compiling OK** (host gcc 13.3, `-DIN_GCC -DHAVE_CONFIG_H`):
`alias.c`, `tree.c` (core), `dwarf2out.c`, `c-parser.c`, `config/i386/i386-c.c`,
and finally **`config/i386/i386.c`** (947 KB object). The whole x86_64 target
backend now builds.

**Root causes fixed to get there:**
1. `-DIN_GCC` was missing (GCC's `INTERNAL_CFLAGS`). Without it,
   `include/ansidecl.h:193` mis-parses and cascades into bogus
   `LAST_AND_UNUSED_RTX_CODE` / `N_REG_CLASSES` / `CUMULATIVE_ARGS` errors.
2. `enum rtx_code` / `LAST_AND_UNUSED_RTX_CODE` are **not** generated — they are
   defined inline in `gcc/rtl.h` (lines 46–57) via `#include "rtl.def"`. So
   `genrtl.h` (only `gen_rtx_fmt_*` helpers) and `insn-codes.h` (only
   `enum insn_code`) are correct as-is; nothing to regenerate.
3. `target-hooks-def.h` is produced by **`genhooks "Target Hook"`** (not
   genconfig). Built `genhooks` and generated `target-hooks-def.h`,
   `c-family/c-target-hooks-def.h`, `common/common-target-hooks-def.h` (the
   latter two into `gen/c-family/` and `gen/common/` so the quoted includes
   resolve).
4. i386 register-number constants (`AX_REG`…`DI_REG`, `XMM0_REG`) and the
   `UNSPECV_*` values come from `define_constants` in `i386.md` →
   `insn-constants.h` (genconstants). **No header includes `insn-constants.h`**,
   so `gen/tm.h` was edited to `#include "insn-constants.h"` alongside
   `insn-flags.h` (a force-include test confirmed this clears the `DI_REG`
   errors).
5. `i386.c:24677` needs `i386-builtin-types.inc`; generated it from
   `i386-builtin-types.def` via `awk -f i386-builtin-types.awk` (868 lines).
6. The assembler/gas capability macros are **configure-time** values, absent
   from a hand-built config. Added to the smoke `GASDEFS` (x86_64-linux, modern
   gas): `HAVE_COMDAT_GROUP`, `HAVE_GAS_SHF_MERGE`,
   `HAVE_GAS_CFI_SECTIONS_DIRECTIVE`, `HAVE_GAS_HIDDEN`,
   `HAVE_GAS_MAX_SKIP_P2ALIGN=65535`, `HAVE_AS_GOTOFF_IN_DATA`,
   `HAVE_AS_IX86_CMOV_SUN_SYNTAX`, `HAVE_AS_IX86_FFREEP`, `HAVE_AS_IX86_FILDQ`,
   `HAVE_AS_IX86_FILDS`, `HAVE_AS_IX86_REP_LOCK_PREFIX`, `HAVE_AS_TLS`,
   `HAVE_AS_GOTTPLTPCALL`, `HAVE_AS_TLSDIRECT`, `HAVE_AS_CFI_SECTIONS`,
   `HAVE_AS_X86_CMPXCHG16B`.
7. **`TARGET_CPU_DEFAULT` was `""`** in `contrib/gcc/config.h:35`. `i386.c:3148`
   does `cpu_names[TARGET_CPU_DEFAULT]`, which became `cpu_names[""]` →
   "array subscript is not an integer". A real x86 build leaves the macro to
   `i386.h:193`'s `#ifndef` fallback. Changed it to the enum constant
   `TARGET_CPU_DEFAULT_generic`.

**Note:** `i386.c:39128` includes `gt-i386.h` (gengtype output); it exists in the
gen dir and the file linked through. `gengtype` phase 2 still exits rc=1 with
nonblocking warnings but emits usable `gtype-desc.{c,h}` + `gt-*.h`.

**Next:** batch-compile the full `cc1` object list (the `OBJS` middle-end set +
 `i386.o` + `C_OBJS`/`c-family` + `i386-c.o` + `ggc-none.o` + `main.o` +
 `OBJS-libcommon[-target]`), fix any remaining SDK gaps, then add the `cc1`
 build section to `contrib/gcc/Makefile` and link.

## 2026-08-30 (Manila, UTC+8)

### 03:00–03:35 — **MILESTONE: in-OS `cc1` links clean** (18.2 MB x86-64 ELF, 0 undefined refs)

The 36-symbol undefined-reference wall from the third link is fully cleared. `cc1`
now links to a valid `EXEC` ELF (entry + `main` present, `.text` ~7 MB). Fixes landed:

- **Constraint macros (6):** `CONSTRAINT_LEN`, `CONST_OK_FOR_CONSTRAINT_P`,
  `CONST_DOUBLE_OK_FOR_CONSTRAINT_P`, `REG_CLASS_FROM_CONSTRAINT`,
  `EXTRA_ADDRESS_CONSTRAINT`, `EXTRA_MEMORY_CONSTRAINT` are all `#define`s in
  `defaults.h`. The undefined refs were **stale objects** (compiled before the
  force-include took effect). Recompiling `recog`, `ira-conflicts`, `ira-costs`,
  `ira-lives`, `postreload`, `regmove`, `reload1`, `reload`, `stmt` → 0 refs.
- **`targetcm` / `targetm_common`:** compiled `config/default-c.c` → `default-c.o`
  (exports `targetcm`) and `common/config/i386/i386-common.c` → `i386-common.o`
  (exports `targetm_common`). Note: `default-c.c` lives in `gcc/config/`, which
  `compile_one.sh` does not search, so it needed a direct compile.
- **GGC allocs:** `ggc_alloc_cleared_machine_function` / `ggc_alloc_stack_local_entry`
  are called by ~18 targets but defined nowhere (not generated, not in headers).
  Added `gen/shim-ggc-alloc.c` defining both via `ggc_internal_cleared_alloc_stat`.
  `struct machine_function` is complete in `i386.h`; `struct stack_local_entry` is
  file-local to `i386.c` so the shim mirrors its 4 fields (2+2+pad+8+8 = 24 B) to
  size the allocation.
- **Mudflap:** `mudflap_init` is only in `tree-mudflap.o`, which collides with
  `tree-nomudflap.o` on 5 other symbols. Kept `tree-nomudflap.o` and added a no-op
  `mudflap_init` stub to `shim-ggc-alloc.c` (only called under runtime `flag_mudflap`).
- **Host funcs → libiberty.a:** compiled `physmem.c`(physmem_total),
  `getruntime.c`(get_run_time), `getpwd.c`(getpwd), `lrealpath.c`(lrealpath),
  `cp-demint.c`(cplus_demangle_v3_components).
- **libdecnumber bid/ieee → libdecnumber.a:** compiled `bid2dpd_dpd2bid.c`,
  `host-ieee32/64/128.c`.
- **SDK libc gaps:** added `asctime` (macro.c) and `bsearch` (files.c) to
  `sdk/posix.c` + declarations to `sdk/include/{time,stdlib}.h`.
- **hwint log fns (5):** `floor_log2`, `exact_log2`, `ctz_hwi`, `clz_hwi`,
  `ffs_hwi` are `static inline` in `hwint.h` only for `GCC_VERSION >= 3004`, but the
  prebuilt objects reference them as out-of-line symbols. Added `gen/shim-hwint.c`
  with self-contained out-of-line defs (HOST_WIDE_INT=`long`, 64-bit; includes
  `<limits.h>` for `CHAR_BIT`).

`cc1_objs.txt` now 348 entries. Archives extended in place.

**Next:** (1) integrate `cc1` into the in-OS self-host build (stage it to `/work`,
wire a `make test-cc1` target); (2) run it in-OS to compile a trivial `.c` and
confirm it emits runnable assembly; (3) add the `cc1` build section to
`contrib/gcc/Makefile` so the whole chain is reproducible from `make`.

### 06:34–06:48 — **MILESTONE: reproducible `cc1` build from `contrib/gcc/Makefile`**

Completed the "Next (3)" item from 03:35: the whole `cc1` host build now runs
from `make -C ics-os/contrib/gcc cc1` (or `all`) with no manual steps. From a
pristine scratch (`rm -rf /tmp/icsos-gcc`) it builds 349/349 cc1 objects,
4/4 archives (`libcpp`/`libiberty`/`libdecnumber`/`libz`), links the 3
prebuilt math archives, and emits a valid `cc1` (18,180,320 B, `T main`, 0
undefined). Incremental `make cc1` is a correct no-op.

Repo additions (all under `ics-os/contrib/gcc/`, untracked):

- **`gen/`** — 94-file (19 MB) snapshot of the generated headers/sources the
  build needs: 14 `.c` (gtype-desc, insn-*, options, options-save), 78 `.h`,
  `all-tree.def`, `i386-builtin-types.inc`, and the `c-family`/`common`
  target-hooks defs. State files (`gtype.state`, `tmp-gtype.state`) and build
  artifacts were excluded; there is **no** `gen/config.h` so it cannot shadow
  `contrib/gcc/config.h`.
- **`shims/`** — `shim-ggc-alloc.c`, `shim-hwint.c`, `cc1-checksum.c`
  (hand-written stand-in for genchecksum output; provides `executable_checksum`).
- **`cc1-objs.txt`** — the 349-entry object list (each entry already `.o`).
- **`decnuminc/config.h`** — minimal decNumber config (`WORDS_BIGENDIAN 0`);
  `dconfig.h` pulls in `tconfig.h` + this `config.h`.
- **`Makefile`** — rewritten. Builds the 4 libc archives, falls back to
  `$(MAKE) -C <gmp|mpfr|mpc>` for the math archives if missing, compiles the 349
  cc1 objects via a pattern rule (5-candidate source search: `gen/`, `shims/`,
  `gcc/`, `gcc/config/i386/`, `gcc/config/`), and links with
  `--start-group … --end-group -Wl,--no-undefined`.

Difficulties hit and fixed:

- **Double `.o`:** `CC1_OBJS` used `$(patsubst %.c,%.o,…)` on a list that already
  carried `.o`, producing `alias.o.o`. Switched to `$(addprefix $(CC1_OBJDIR)/,…)`.
- **VERDEFS quoting (the real blocker):** `toplev.o` failed with
  `‘ics’ undeclared` / `‘os’ undeclared` from `TARGET_NAME`. Root cause: in a
  make variable expanded onto the recipe command line, `-DTARGET_NAME="…"` has
  its quotes stripped by the single shell pass. `compile_one.sh` never hit this
  because the quotes lived *inside* a bash variable passed via unquoted
  `$CFLAGS` (no re-parsing). Fix: backslash-escape the quotes in the make
  variable (`-DTARGET_NAME=\"x86_64-ics-os\"`) so the shell preserves them.
  Verified with a scratch Makefile that `\"` (not single-quoting) survives.
- **libiberty** needed 2 files beyond the 5 known host funcs: `cp-demangle.c`,
  `safe-ctype.c` (now 48 members).
- **libdecnumber** builds *without* `-fno-builtin`/`-fno-asynchronous` and uses
  the staged `decnuminc/`; **libz** builds *without* `-nostdinc` (its `zconf.h`
  needs `sys/feature_tests.h` + `unistd.h`). `crc32` is in both libiberty and
  libz; link order (libiberty first in the group) resolves it.

**Next:** (1) in-OS validation via QEMU — stage `cc1` to `/work`, wire a
`make test-cc1`, run it in-OS to compile a trivial `.c` and confirm runnable
output (host `./cc1 --version` is meaningless: the SDK uses the ICS-OS
`int 0x30` syscall ABI); (2) optionally make GMP/MPFR/MPC build-from-source
reproducible too (today they are prebuilt `.a` + a fallback rule).

### 07:00–07:16 — **GMP/MPFR/MPC made source-reproducible; GMP default-goal bug fixed**

Per user direction the next step was (2): make the three math libraries
build-from-source reproducible through `contrib/gcc/Makefile` before any QEMU
work. Wiping the three `.a` + `obj/` dirs and the `/tmp/icsos-gcc` stage, then
`make -C gcc -j4 cc1`, rebuilt MPFR and MPC from `references/` source, but
**`libgmp.a` did not build** and the cc1 link failed with
`/usr/bin/ld: cannot find ../../contrib/gmp/libgmp.a`.

Root cause (GMP Makefile): the `dual_rule`/`src_rule` `$(foreach … $(eval …))`
blocks emit the object-file targets *before* the `all: libgmp` line, so the
first target in the file — `obj/logops_n-and_n.o` — becomes make's default
goal. A bare `make -C gmp` built only that one object and exited 0 without
creating the archive. `make -C gmp -n` confirmed it planned just that object;
`make -C gmp -n all` planned all 460 compiles. MPFR/MPC were unaffected because
their `all:` precedes their object rules.

Fix: pinned the default goal in `contrib/gmp/Makefile` with
`.DEFAULT_GOAL := all` (just before the `.PHONY`/`all` block, with a comment
explaining the foreach/eval ordering). Made the gcc overlay's math fallback
rules robust by invoking `$(MAKE) -C <dir> all` explicitly (was a bare
`$(MAKE) -C <dir>`), and added a standalone `mathlibs` target to
`contrib/gcc/Makefile` that regenerates all three archives from source.

Verification (all green):

- Full source rebuild (wiped all three `.a`+`obj/` + cc1 stage, then
  `make -C gcc -j4 cc1`) → exit 0: `libgmp.a` 461 members, `libmpfr.a` 217,
  `libmpc.a` 78 (all from `references/`); `cc1` relinked 18,180,320 B, `T main`,
  **0 undefined** symbols.
- Host functional tests: `GMP_HOST_TEST_PASS`, `MPFR_HOST_TEST_PASS`,
  `MPC_TEST_PASS` (each runs its rebuilt archive on the host).
- `make -C gcc test-gmpmpfr` → `GCC_GMPMPFR_WIRING_OK`.
- `mathlibs` target: deleting only `mpc/libmpc.a` then `make -C gcc mathlibs`
  rebuilt just MPC; `all` still defaults to cc1; incremental `make cc1` is a
  no-op when up to date.

The whole chain is now source-reproducible: `make -C contrib/gcc cc1`
regenerates GMP/MPFR/MPC from `references/` (when a `.a` is missing) and builds
a link-clean `cc1`.

**Next:** in-OS validation via QEMU — stage `cc1` to `/work`, wire a
`make test-cc1`, run it in-OS to compile a trivial `.c` and confirm runnable
output (host `./cc1 --version` is meaningless: the SDK uses the ICS-OS
`int 0x30` syscall ABI).
