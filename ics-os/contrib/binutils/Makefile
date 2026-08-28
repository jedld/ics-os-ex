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
DEFS  := -DHAVE_CONFIG_H -DI8086 -DDEFAULT_ARCH=elf64_littleswap
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
LIBBFD_C := \
	libbfd.c archures.c targets.c section.c syms.c reloc.c \
	hash.c linker.c cpu-i386.c \
	bfd.c bfdio.c bfdwin.c cache.c \
	archive.c archive64.c coffgen.c corefile.c \
	format.c init.c opncls.c \
	srec.c binary.c tekhex.c ihex.c verilog.c \
	stabs.c stab-syms.c merge.c compress.c simple.c \
	dwarf2.c \
	elf.c elflink.c \
	elf64.c elf64-gen.c elf64-x86-64.c \
	elf-strtab.c elf-eh-frame.c elf-attrs.c elf-ifunc.c

LIBBFD_O := $(patsubst %.c,$(OBJ)/libbfd/%.o,$(LIBBFD_C))

# ---- libiberty (self-contained foundation, no libbfd) ----
# Curated: drop C++ demanglers, non-ELF simple-object backends, and Windows
# process-exec backends. Everything else the tools may reference is kept.
LIBIBERTY_C := \
	alloca.c argv.c asprintf.c atexit.c basename.c bcmp.c bcopy.c \
	bsearch.c bzero.c calloc.c choose-temp.c clock.c concat.c \
	crc32.c dwarfnames.c dyn-string.c fdmatch.c ffs.c fibheap.c \
	filename_cmp.c floatformat.c fnmatch.c fopen_unlocked.c getcwd.c \
	getopt.c getopt1.c getpwd.c getruntime.c \
	hashtab.c hex.c index.c insque.c lbasename.c \
	make-relative-prefix.c make-temp-file.c md5.c memchr.c \
	memcmp.c memcpy.c memmem.c memmove.c mempcpy.c memset.c mkstemps.c \
	objalloc.c obstack.c partition.c pexecute.c pex-common.c \
	pex-one.c pex-unix.c physmem.c putenv.c random.c regex.c rename.c \
	rindex.c safe-ctype.c setenv.c setproctitle.c sha1.c sigsetmask.c \
	simple-object.c simple-object-elf.c snprintf.c sort.c spaces.c \
	splay-tree.c stack-limit.c stpcpy.c stpncpy.c strcasecmp.c \
	strchr.c strdup.c strerror.c strncasecmp.c strncmp.c strrchr.c \
	strsignal.c strstr.c strtod.c strtol.c strtoul.c strndup.c \
	strverscmp.c timeval-utils.c tmpnam.c unlink-if-ordinary.c \
	vasprintf.c vfork.c vfprintf.c vprintf.c vsnprintf.c vsprintf.c \
	waitpid.c xatexit.c xexit.c xmalloc.c xmemdup.c xstrdup.c \
	xstrerror.c xstrndup.c

LIBIBERTY_O := $(patsubst %.c,$(OBJ)/libib/%.o,$(LIBIBERTY_C))

.PHONY: libiberty libbfd all ar as ld clean

all: libiberty libbfd

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

ar: libiberty
	@echo "ar: (libbfd + ar next)"

as: libiberty
	@echo "as: (libbfd + gas next)"

ld: libiberty
	@echo "ld: (libbfd + ld next)"

clean:
	rm -rf $(STAGE)
