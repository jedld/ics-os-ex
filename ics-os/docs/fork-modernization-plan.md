# Fork modernization plan

Status: Phase 2 copy-on-write fork implemented and validated on x86-64, with
active user and syscall stacks still eagerly copied under the current CPL0 ABI.

## Current behavior

SDK `fork()` invokes DEX syscall `0x90`. The x86-64 interrupt wrapper sends that
request directly to `user_fork_frame()`, bypassing the legacy dispatcher. The
kernel COW-shares ordinary writable 4 KiB user pages, writes zero into the
eagerly copied child syscall frame, constructs a fresh PCB and context, clones
resources, and publishes the child only after every acquisition succeeds. ELF
text remains read-only. The active user and syscall stacks are eagerly copied
because user execution and kernel syscall frames currently share a CPL0 stack.
The parent receives the child PID and the child resumes from the copied frame
with `rax=0`.

VFS, block, and io_uring descriptors acquire typed owner references under the
parent fd lock and roll back as a unit. Fork rejects a process with another
thread, a non-private address space, an in-flight io_uring operation, or no
remaining retained-status capacity. The old two-level `forkprocess()` is
compiled out on x86-64.

## Resolved Phase 1 gaps

- The x86-64 syscall no longer uses the dispatcher, 32-bit page copier, global
  paging disable, PCB `memcpy`, or PID-comparison return heuristic.
- Sparse private leaves and the saved syscall frame are eagerly copied with
  reverse-order PML4 rollback; ELF64 loading fails rather than falling back to
  a shared address space after allocation failure.
- PCB, parameters, memory metadata, FPU state, and typed descriptors are cloned
  explicitly. Descriptor failure unwinds every inherited slot.
- PID allocation, parent accounting, and scheduler insertion are one final
  IRQ-safe publication step. Targeted wait rejects non-children.
- Every accepted fork has retained exit-status capacity; exhaustion returns
  `EAGAIN` before cloning rather than silently dropping a later child status.
- The focused guest gate and 1/2/4/8-vCPU matrix cover return ABI, isolation,
  inherited fd behavior, explicit exit status, non-child wait, and delayed
  reaping of ten children.

Remaining limitations are explicit rejection of multithreaded callers and
in-flight io_uring. General fork allocation-failure injection, orphan policy,
and a dynamically allocated zombie representation remain QA and lifecycle
enhancements. The invalid legacy 32-bit COW handler is not used.

## Phase 1: safe eager-copy fork (implemented)

Eager copy is the shortest correct implementation because current VM ownership
already assumes one address space owns every private leaf frame.

### Syscall and context contract

- Implement fork synchronously from the syscall/trap path rather than through
  the legacy dispatcher queue.
- Capture an explicit syscall trap frame. Clone it into the child, set child
  `rax=0`, and return the child PID in the parent's frame. Do not infer the
  return side from `current_process->processid`.
- Initially reject fork from a process with more than one thread (`-EAGAIN`). A
  later stop-the-world protocol may snapshot one caller while discarding other
  threads, matching POSIX semantics.
- Save the caller's live FPU state before cloning it.

### Address-space contract

- Add `userpd_clone_eager(parent, &child)`. Create a private PML4 with
  `userpd_create()`, walk only private user PTE tables in PDPT[0], allocate a
  child leaf for every present parent leaf, and copy through `KDIRECT()`.
- Preserve user, writable, executable/NX, and relevant software flags. Continue
  sharing only the documented kernel mappings.
- Hold an address-space mutation lock across the snapshot. `brk`, `mmap`,
  `munmap`, page faults, and fork must use the same lock.
- Return a precise error and use `userpd_free()` for reverse-order rollback.
  Never publish a partially cloned address space.

### Process and resource contract

- Allocate and zero a new PCB. Explicitly copy credentials, cwd, session,
  process group, signal dispositions, FPU state, memory bounds, and the child
  trap context. Reset PID-owned, scheduler, wait, child, message, pending-signal,
  accounting, and queue-link fields.
- Add typed get/put ownership for cwd/mount and every inherited non-fd object.
  Duplicate command-line/environment storage or move it to a refcounted process
  image object. Do not inherit raw legacy stream pointers.
- Make fd-table cloning all-or-nothing with rollback. Define io_uring policy:
  initially drain/cancel requests that retain user addresses, or reject fork
  while such requests exist.
- Allocate PID and increment parent live-child accounting only at commit. Insert
  the fully initialized child into the scheduler as the final operation.
- On any failure, drop references and frames in reverse acquisition order and
  leave parent-visible state unchanged.

## Phase 2: copy-on-write (implemented)

The x86-64 implementation provides:

- global per-frame reference counts integrated with frame allocation/release;
- a VM lock around fork snapshot/protection and COW fault mutation, plus a
  software PTE bit recording originally writable COW mappings;
- atomic parent/child write-protection followed by cross-CPU TLB shootdown;
- page-fault validation for present, user, write-protection faults only;
- allocate-copy-remap under the PTE lock, with a one-owner fast path;
- decref-aware unmap, exec, exit, and rollback;
- rejection of in-flight io_uring buffers, eager stack copying under the CPL0
  ABI, and preservation of executable segment permissions;
- counters and traces for COW faults, copies, fast paths, shootdowns, and OOM.

`CR0.WP` is enabled on the BSP and APs so kernel-mode writes obey read-only
PTEs. A one-shot fault-injection hook validates COW allocation failure without
publishing a corrupted mapping. The synchronous shootdown path targets CPUs by
authoritative address-space ownership; current BSP-pinned user processes cover
the local path, while migration support must exercise its remote path.

## Required QA

Phase 1 must include:

- parent returns child PID; child returns zero exactly once;
- private stack, heap, data, BSS, sparse mmap, and unmapped-hole isolation;
- inherited shared file offsets, parent-close/child-use, close-on-exec policy,
  cwd, session, process group, and signal-state checks;
- child `exec`, normal/abnormal exit, targeted and any-child wait, non-child
  `ECHILD`, orphan policy, and more than eight simultaneous exits;
- deterministic failure at every PCB, page-table, leaf-frame, metadata, and fd
  acquisition point with zero leaked frames/references or visible child;
- rejection of multithreaded and unsafe in-flight async-I/O cases;
- repeated 1/2/4/8-vCPU fork/write/exit/wait contention with watchdogs and
  structured KTAP/TAP-compatible results.

Phase 2 tests cover parent/child writes to a shared page, single-owner
optimization after child teardown, read-only text faults, and deterministic COW
OOM. Dedicated unmap/exit race stress, TLB-shootdown delay/loss injection, and
DMA-pinned-page cases remain required before expanding fork to those scenarios.

## Acceptance boundary

Phase 2 passed `make test-fork-matrix` under 1/2/4/8 vCPUs. Each run validates
parent/child returns and COW isolation, the one-owner fast path, immutable text,
COW OOM recovery, inherited fd state, non-child `waitpid` rejection, explicit
exit status, and ten-child delayed-reap pressure. `make test-integration` and
`make test-spawn` also pass. `fork()` is supported within the restrictions
above; `posix_spawn()` remains preferred for toolchain process creation.
`vfork()` remains `ENOSYS`.