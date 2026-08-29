#
# ICS-OS host build of GNU binutils (2.23) as/ld/ar against the ICS-OS SDK.
#
# Same pattern as contrib/gnumake: host gcc, -nostdlib -ffreestanding -nostdinc,
# -I sdk/include + our hand-written config.h, link the app + SDK .c files into a
# single static ELF64 user executable that runs inside ICS-OS.
#
# Phase 1 order: libiberty -> libbfd (ELF x86-64 subset) -> ar -> as -> ld.
# Each stage is a host-build + (later) in-OS run. The host build is the gap
# detector: anything libiberty/libbfd/the tools need that the SDK lacks shows up
# here as a missing symbol/header.
#

include ../../sdk/app.mk

NAME      := binutils
CC        := gcc
SDK       := ../../sdk
ICSOS_ROOT:= ../..
SRC       ?= ../../../references/binutils-2.23

STAGE     := /tmp/icsos-binutils
OBJ       := $(STAGE)/obj

# Our config.h must be found as <config.h> and as the -I dir for binutils.
CONF := $(CURDIR)/config.h
CONFDIR := $(CURDIR)

# Common: SDK headers first (override host libc), then our config, then the
# binutils include trees. -DHAVE_CONFIG_H so sources include "config.h".
# SELECT_VECS restricts libbfd's _bfd_target_vector[] (targets.c) to exactly the
# vectors we build, so no other backend vector symbols are referenced. Only the
# plain ELF64 x86-64 vector (the l1om/k1om Larrabee/NACL ABIs are dropped: they
# pull in elf32 NACL core-file support we don't need).
# bin_dummy_emulation is #define'd to the selected emulation vector
# (upstream: binutils/Makefile.in -Dbin_dummy_emulation=$(EMULATION_VECTOR));
# bin_vanilla_emulation is defined in emul_vanilla.c.
DEFS  := -DHAVE_CONFIG_H -DI8086 -DDEFAULT_ARCH=\"x86_64\" \
	-DSELECT_VECS='&bfd_elf64_x86_64_vec' \
	-Dbin_dummy_emulation=bin_vanilla_emulation
INC   := -nostdinc -I$(SDK)/include -I$(CONFDIR) \
         -I$(SRC)/include -I$(SRC)/bfd -I$(SRC)/libiberty \
         -I$(SRC)/binutils -I$(SRC)/gas -I$(SRC)/ld
CFLAGS := $(APP_CFLAGS) $(DEFS) $(INC)

SDKLIBS := $(SDK)/tccsdk.c $(SDK)/posix.c $(SDK)/libtcc1.c $(SDK)/crt1.c $(SDK)/setjmp.c

# ---- libbfd (ELF64 x86-64 subset + core) ----
# Core (always) + the one ELF64 x86-64 target + general-ELF support. No other
# backends (COFF/a.out/PE/Mach-O/VMS/...), no ia64/mips/sparc xx files, no
# plugin, no NLS. Generated headers (bfd.h, bfdver.h, bfd_stdint.h, targmatch.h,
# elf64-target.h) are committed here and found via -I$(CONFDIR) first.

# All cpu-*.c are static architecture info tables (self-contained: only
# sysdep.h/bfd.h/libbfd.h/libiberty.h; cpu-ia64.c also pulls opcodes/ia64-opc.h).
# archures.c's master bfd_arch_list[] references every bfd_*_arch, so we build
# the full set (same as a normal upstream libbfd) rather than trimming archures.c.
# cpu-ia64-opc.c is excluded: it is #include'd by cpu-ia64.c, not standalone.
CPUC = $(filter-out cpu-ia64-opc.c,$(notdir $(wildcard $(SRC)/bfd/cpu-*.c)))
LIBBFD_C := \
	libbfd.c archures.c targets.c section.c syms.c reloc.c \
	hash.c linker.c \
	bfd.c bfdio.c bfdwin.c cache.c \
	archive.c archive64.c coffgen.c corefile.c \
	format.c init.c opncls.c \
	srec.c binary.c tekhex.c ihex.c verilog.c \
	stabs.c stab-syms.c merge.c compress.c simple.c \
	dwarf2.c dwarf1.c \
	elf32.c \
	elf.c elflink.c \
	elf64.c elf64-gen.c elf64-x86-64.c elf-nacl.c \
	elf-strtab.c elf-eh-frame.c elf-attrs.c elf-ifunc.c \
	aout-ns32k.c \
	$(CPUC)

LIBBFD_O := $(patsubst %.c,$(OBJ)/libbfd/%.o,$(LIBBFD_C))

# ---- libiberty (self-contained foundation, no libbfd) ----
# Curated: drop C++ demanglers, non-ELF simple-object backends, and Windows
# process-exec backends. Everything else the tools may reference is kept.
# IMPORTANT: the SDK (tccsdk.c / posix.c) is the canonical libc runtime and
# already defines a set of libc symbols. libiberty's own copies of THOSE must
# be omitted here, or the link fails with "multiple definition". The 25 dropped
# files (calloc clock getcwd memchr memcmp memcpy memmove memset putenv rename
# setenv snprintf strchr strdup strncmp strrchr strtod strtol strtoul vfork
# vfprintf vprintf vsnprintf) are provided by the SDK instead.
LIBIBERTY_C := \
	alloca.c argv.c asprintf.c atexit.c basename.c bcmp.c bcopy.c \
	bsearch.c bzero.c choose-temp.c concat.c \
	crc32.c dwarfnames.c dyn-string.c fdmatch.c ffs.c fibheap.c \
	filename_cmp.c floatformat.c fnmatch.c fopen_unlocked.c \
	getopt.c getopt1.c getpwd.c getruntime.c \
	hashtab.c hex.c index.c insque.c lbasename.c lrealpath.c \
	make-relative-prefix.c make-temp-file.c md5.c \
	memmem.c mempcpy.c mkstemps.c \
	objalloc.c obstack.c partition.c pexecute.c pex-common.c \
	pex-one.c pex-unix.c physmem.c random.c regex.c \
	rindex.c safe-ctype.c setproctitle.c sha1.c sigsetmask.c \
	simple-object.c simple-object-elf.c sort.c spaces.c \
	splay-tree.c stack-limit.c stpcpy.c stpncpy.c strcasecmp.c \
	strerror.c strncasecmp.c \
	strsignal.c strndup.c \
	strverscmp.c timeval-utils.c tmpnam.c unlink-if-ordinary.c \
	vasprintf.c vsprintf.c \
	xatexit.c xexit.c xmalloc.c xmemdup.c xstrdup.c \
	xstrerror.c xstrndup.c

# ---- binutils common support for ar/ranlib ----
# bucomm (program_name/fatal/list_supported_targets), version (print_version),
# filemode, binemul (ar_emul_* dispatch), emul_vanilla (default emulation),
# arparse.c + arlex.c (bison/flex-generated, checked into the 2.23 tarball),
# not-ranlib.c (int is_ranlib = 0), rename.c (smart_rename).
AR_C := \
	ar.c arsup.c arparse.c arlex.c binemul.c emul_vanilla.c \
	bucomm.c version.c filemode.c not-ranlib.c rename.c

LIBIBERTY_O := $(patsubst %.c,$(OBJ)/libib/%.o,$(LIBIBERTY_C))
AR_O        := $(patsubst %.c,$(OBJ)/ar/%.o,$(AR_C))
AR_EXE      := $(CURDIR)/ar.exe

# ---- opcodes (instruction-encoding library) ----
# i386-opc.c (the i386/x86-64 opcode + modifier tables, used by GAS) and the
# generic disassembler core (dis-buf/disassemble/dis-init) + i386-dis.c, built
# so objcopy/ld can be added later. i386-tbl.h / i386-init.h are pre-generated
# and checked into the tarball (opcodes/).
OPCODES_C := \
	i386-opc.c i386-dis.c \
	dis-buf.c disassemble.c dis-init.c

# ---- GAS (the assembler core) ----
# The 31-file core from gas/Makefile.in GAS_CFILES, plus the target files,
# which live in gas/config/ and are handled by a separate rule:
#   obj-elf.c   - the ELF object format (obj-elf.h maps the generic obj_*
#                 names to obj_elf_*; no te-elf.h/targ-elf.h in 2.23)
#   atof-ieee.c - IEEE float literals (tc-i386.c's md_atof calls ieee_md_atof)
#   tc-i386.c   - the i386/x86-64 CPU backend (#includes tc-i386-intel.c, so
#                 that file is NOT listed separately). It does not reference
#                 the disassembler, so as.exe only needs i386-opc from the
#                 opcodes lib.
# NOTE: itbl-ops.c / itbl-lex-wrapper.c / cgen.c are NOT built. Those serve
# the cgen instruction-table backends (MIPS only, gated by HAVE_ITBL_CPU).
# i386 has no itbl table, so as.c compiles with `#define itbl_init()` (a
# no-op) and the itbl objects are absent from the link.
GAS_C := \
	app.c as.c atof-generic.c compress-debug.c cond.c depend.c \
	dwarf2dbg.c dw2gencfi.c ecoff.c ehopt.c expr.c \
	flonum-copy.c flonum-konst.c flonum-mult.c frags.c hash.c \
	input-file.c input-scrub.c listing.c literal.c macro.c \
	messages.c output-file.c read.c remap.c sb.c stabs.c subsegs.c \
	symbols.c write.c
GAS_CFG_C := obj-elf.c atof-ieee.c tc-i386.c

OPCODES_O   := $(patsubst %.c,$(OBJ)/opcodes/%.o,$(OPCODES_C))
GAS_O       := $(patsubst %.c,$(OBJ)/gas/%.o,$(GAS_C))
GAS_CFG_O   := $(patsubst %.c,$(OBJ)/gas/%.o,$(GAS_CFG_C))
AS_EXE      := $(CURDIR)/as.exe

# ---- ld (the linker) ----
# Core from ld/Makefile.in CFILES + GENERATED_CFILES. The bison/flex outputs
# (ldgram.c, ldlex.c, deffilep.c) are checked into the 2.23 tarball, so no
# yacc/lex run is needed. The two *generated* build artifacts we provide
# ourselves:
#   eelf_x86_64.c  - the elf_x86_64 emulation. Upstream it is produced by
#                    genscripts.sh (emulparams/elf_x86_64.sh + emultempl/elf.em
#                    + scripttempl/elf.sc). We ran that script once and commit
#                    the result here (found via -I$(CONFDIR) before $(SRC)/ld).
#   ldemul-list.h  - the EMULATION_LIST; for our single target it is just
#                    &ld_elf_x86_64_emulation (hand-written, same shape as the
#                    Makefile.in rule produces).
# ldemul.c does `#include "ldemul-list.h"` and it must find OUR single-target
# list (CONFDIR) before any in $(SRC)/ld — the -I order already guarantees
# that.
LD_C := \
	ldctor.c ldemul.c ldexp.c ldfile.c ldlang.c \
	ldmain.c ldmisc.c ldver.c ldwrite.c lexsup.c \
	mri.c ldcref.c pe-dll.c pep-dll.c ldlex-wrapper.c \
	ldgram.c deffilep.c
LD_EMU_C := eelf_x86_64.c

LD_O   := $(patsubst %.c,$(OBJ)/ld/%.o,$(LD_C))
LD_EMU := $(OBJ)/ld/eelf_x86_64.o
LD_EXE := $(CURDIR)/ld.exe

# ld core includes live in ld/ (in the global INC). The emulation file
# includes "elf/common.h" / "elf-bfd.h" (in $(SRC)/bfd, also global). No extra
# include dirs are required beyond the global INC + CONFDIR (for ldemul-list.h
# and eelf_x86_64.c).

# GAS includes use the repo-root prefix ("bfd/elf-bfd.h", "opcodes/...") and
# the gas/config dir for target headers, which the global INC does not have.
# $(OBJ)/gas is added so the generated target headers below are found.
GASINC := -I$(OBJ)/gas -I$(SRC) -I$(SRC)/gas/config

# as.h includes "targ-env.h", te-*.h includes "obj-format.h", and obj-elf.h
# includes "targ-cpu.h"; upstream makes all of these as build-dir copies of the
# selected te-<te_file>.h / obj-<obj_format>.h / tc-<cpu>.h. For x86-64
# ELF/Linux that is te-linux.h / obj-elf.h / tc-i386.h.
GAS_GEN_H := $(OBJ)/gas/targ-env.h $(OBJ)/gas/obj-format.h $(OBJ)/gas/targ-cpu.h

$(OBJ)/gas/targ-env.h: $(SRC)/gas/config/te-linux.h
	@mkdir -p $(OBJ)/gas
	cp $< $@
$(OBJ)/gas/obj-format.h: $(SRC)/gas/config/obj-elf.h
	@mkdir -p $(OBJ)/gas
	cp $< $@
$(OBJ)/gas/targ-cpu.h: $(SRC)/gas/config/tc-i386.h
	@mkdir -p $(OBJ)/gas
	cp $< $@

.PHONY: libiberty libbfd opcodes gas all ar as ld clean

all: ar as ld

libiberty: $(LIBIBERTY_O)
	@echo "libiberty: $(words $(LIBIBERTY_O)) objects built OK"

$(OBJ)/libib/%.o: $(SRC)/libiberty/%.c $(CONF)
	@mkdir -p $(OBJ)/libib
	$(CC) $(CFLAGS) -c $< -o $@

libbfd: $(LIBBFD_O)
	@echo "libbfd: $(words $(LIBBFD_O)) objects built OK"

$(OBJ)/libbfd/%.o: $(SRC)/bfd/%.c $(CONF)
	@mkdir -p $(OBJ)/libbfd
	$(CC) $(CFLAGS) -c $< -o $@

# Compile the ar support files (arparse/arlex are bison/flex-generated but are
# checked into the 2.23 tarball, so no bison/flex run is needed).
$(OBJ)/ar/%.o: $(SRC)/binutils/%.c $(CONF)
	@mkdir -p $(OBJ)/ar
	$(CC) $(CFLAGS) -c $< -o $@

# ar: link the tool + libbfd + libiberty + the SDK runtime into a single
# static ELF64 user executable (same pattern as contrib/gnumake/make.exe).
# CFLAGS is passed on the link line too (gnumake does the same) so the SDK
# .c files (which #include <...>) are found via the -I paths.
ar: $(AR_EXE)

$(AR_EXE): $(AR_O) $(LIBBFD_O) $(LIBIBERTY_O) $(SDKLIBS) \
	   $(CURDIR)/fileno-alias.c $(CURDIR)/demangle-stub.c
	$(CC) $(CFLAGS) $(APP_LDFLAGS) -o$@ \
		$(AR_O) $(LIBBFD_O) $(LIBIBERTY_O) $(SDKLIBS) \
		$(CURDIR)/fileno-alias.c $(CURDIR)/demangle-stub.c
	@echo "ar: linked $(AR_EXE)"

install: ar as ld
	mkdir -p $(ICSOS_ROOT)/apps
	cp $(AR_EXE) $(AS_EXE) $(LD_EXE) $(ICSOS_ROOT)/apps
	# ld locates its default linker script (ldscripts/elf_x86_64.x*) via
	# find_scripts_dir()'s "next to the binary" fallback, so stage them next
	# to ld.exe. Without them a bare `ld in.o -o out` cannot find a script.
	# All variants are staged: the base script is chosen by link flags
	# (e.g. default combreloc -> elf_x86_64.xc, not the bare .x).
	mkdir -p $(ICSOS_ROOT)/apps/ldscripts
	cp $(CURDIR)/ldscripts/elf_x86_64.x* $(ICSOS_ROOT)/apps/ldscripts/

uninstall:
	rm -f $(ICSOS_ROOT)/apps/ar.exe $(ICSOS_ROOT)/apps/as.exe $(ICSOS_ROOT)/apps/ld.exe
	rm -rf $(ICSOS_ROOT)/apps/ldscripts

# ---- opcodes + GAS (assembler) ----
opcodes: $(OPCODES_O)
	@echo "opcodes: $(words $(OPCODES_O)) objects built OK"

$(OBJ)/opcodes/%.o: $(SRC)/opcodes/%.c $(CONF)
	@mkdir -p $(OBJ)/opcodes
	$(CC) $(CFLAGS) -c $< -o $@

gas: $(GAS_O) $(GAS_CFG_O)
	@echo "gas: $(words $(GAS_O) $(GAS_CFG_O)) objects built OK"

# GAS core sources live in gas/.
$(OBJ)/gas/%.o: $(SRC)/gas/%.c $(CONF) $(GAS_GEN_H)
	@mkdir -p $(OBJ)/gas
	$(CC) $(CFLAGS) $(GASINC) -c $< -o $@

# GAS target sources live in gas/config/.
$(OBJ)/gas/obj-elf.o: $(SRC)/gas/config/obj-elf.c $(CONF) $(GAS_GEN_H)
	@mkdir -p $(OBJ)/gas
	$(CC) $(CFLAGS) $(GASINC) -c $< -o $@
$(OBJ)/gas/atof-ieee.o: $(SRC)/gas/config/atof-ieee.c $(CONF) $(GAS_GEN_H)
	@mkdir -p $(OBJ)/gas
	$(CC) $(CFLAGS) $(GASINC) -c $< -o $@
$(OBJ)/gas/tc-i386.o: $(SRC)/gas/config/tc-i386.c $(CONF) $(GAS_GEN_H)
	@mkdir -p $(OBJ)/gas
	$(CC) $(CFLAGS) $(GASINC) -c $< -o $@

# as: link GAS + opcodes + libbfd + libiberty + the SDK runtime into a single
# static ELF64 user executable (same pattern as ar.exe).
as: $(AS_EXE)

$(AS_EXE): $(GAS_O) $(GAS_CFG_O) $(OPCODES_O) $(LIBBFD_O) $(LIBIBERTY_O) $(SDKLIBS) \
	   $(CURDIR)/fileno-alias.c $(CURDIR)/demangle-stub.c
	$(CC) $(CFLAGS) $(APP_LDFLAGS) -o$@ \
		$(GAS_O) $(GAS_CFG_O) $(OPCODES_O) $(LIBBFD_O) $(LIBIBERTY_O) $(SDKLIBS) \
		$(CURDIR)/fileno-alias.c $(CURDIR)/demangle-stub.c
	@echo "as: linked $(AS_EXE)"

# ld: compile the core objects + the emulation object, then link against
# libbfd + libiberty + the SDK runtime into a single static ELF64 user
# executable (same pattern as ar.exe / as.exe).
ld: $(LD_EXE)

# ld's configure injects the install dirs + default emulation as -D string
# literals. ldfile.c's find_scripts_dir() uses the dirs to locate linker
# scripts (for the in-OS tool the scripts are embedded in eelf_x86_64.c via
# genscripts.sh, so the paths are nominal). DEFAULT_EMULATION is the name
# matched in ldemul.c (must equal the emulation's .name, "elf_x86_64").
LDDEFS := -DSCRIPTDIR=\"/usr/lib\" -DBINDIR=\"/usr/bin\" -DTOOLBINDIR=\"/usr/bin\" \
	-DDEFAULT_EMULATION=\"elf_x86_64\"

# Every ld core file includes ld/sysdep.h before bfd.h, and ld/sysdep.h in turn
# includes config.h (which defines PACKAGE/PACKAGE_VERSION that bfd.h requires).
# The pre-generated ldlex.c is compiled through ldlex-wrapper.c, which includes
# sysdep.h first, so no force-include is needed anywhere.
$(OBJ)/ld/%.o: $(SRC)/ld/%.c $(CONF) $(CURDIR)/ldemul-list.h
	@mkdir -p $(OBJ)/ld
	$(CC) $(CFLAGS) $(LDDEFS) -c $< -o $@

# The emulation file lives in $(CURDIR) (committed generated source), so it is
# compiled directly rather than from $(SRC)/ld.
$(LD_EMU): $(CURDIR)/eelf_x86_64.c $(CONF) $(CURDIR)/ldemul-list.h
	@mkdir -p $(OBJ)/ld
	$(CC) $(CFLAGS) $(LDDEFS) -c $< -o $@

$(LD_EXE): $(LD_O) $(LD_EMU) $(LIBBFD_O) $(LIBIBERTY_O) $(SDKLIBS) \
	   $(CURDIR)/fileno-alias.c $(CURDIR)/demangle-stub.c
	$(CC) $(CFLAGS) $(APP_LDFLAGS) -o$@ \
		$(LD_O) $(LD_EMU) $(LIBBFD_O) $(LIBIBERTY_O) $(SDKLIBS) \
		$(CURDIR)/fileno-alias.c $(CURDIR)/demangle-stub.c
	@echo "ld: linked $(LD_EXE)"

clean:
	rm -rf $(STAGE)
