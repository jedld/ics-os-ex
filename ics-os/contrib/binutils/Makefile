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
DEFS  := -DHAVE_CONFIG_H -DI8086 -DDEFAULT_ARCH=elf64_littleswap \
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

.PHONY: libiberty libbfd all ar as ld clean

all: ar

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

install: ar
	mkdir -p $(ICSOS_ROOT)/apps
	cp $(AR_EXE) $(ICSOS_ROOT)/apps

uninstall:
	rm -f $(ICSOS_ROOT)/apps/ar.exe

as: libiberty
	@echo "as: (libbfd + gas next)"

ld: libiberty
	@echo "ld: (libbfd + ld next)"

clean:
	rm -rf $(STAGE)
