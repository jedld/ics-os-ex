#
# ICS-OS host build of GCC 4.7.4 (C only) against the ICS-OS SDK.
#
# Same pattern as contrib/binutils: host gcc, -nostdlib -ffreestanding
# -nostdinc, -I sdk/include + our hand-written config.h, link the component +
# SDK .c files into a single static ELF64 user executable that runs in-OS.
#
# Pipeline order (each is a host-build stage; the host build is the gap
# detector for what the SDK lacks):
#   1. libcpp    - the C preprocessor core (target-independent)
#   2. libiberty - the GNU common library subset cc1 needs
#   3. GMP       - integer/ration constant arithmetic (built vs SDK)
#   4. MPFR      - float constant arithmetic (built vs SDK)
#   5. libdecnumber / libz - decimal FP + PCH compression, linked into cc1
#   6. cc1       - C frontend + middle end + config/i386 target   [this milestone]
#   7. gcc       - the driver
#   8. in-OS     - compile+link+run a C program with the in-OS toolchain
#
# GMP/MPFR/MPC: the *implementation* is built against the SDK in contrib/gmp
# (libgmp.a, 64-bit limbs), contrib/mpfr (libmpfr.a) and contrib/mpc
# (libmpc.a), then linked into cc1. cc1 must compile against THOSE overlays'
# headers, not the host's: our gmp.h maps the public mpz_*/mpn_* names to the
# __gmp*/__gmpn* symbols libgmp.a actually exports (the host gmp.h uses bare
# names and would leave every mpz_* reference undefined at link time). MPFR
# uses bare mpfr_* names, so its public mpfr.h links directly against
# libmpfr.a.
#
# cc1 generated headers: the 14 insn-*/options/gtype-desc sources and the 78
# generated .h files (gt-*.h, tm.h, genrtl.h, ...) are a deterministic output
# of the frozen GCC 4.7.4 .def set + the generator programs (genrtl/genflags/
# genemit/.../gengtype). They are snapshotted in gen/ so `make cc1` works
# without re-running the full generator pipeline (that pipeline is a separate
# milestone). The three hand-written stand-ins (shim-ggc-alloc.c, shim-hwint.c,
# cc1-checksum.c) live in shims/.
#
include ../../sdk/app.mk

NAME      := gcc
CC        := gcc
AR        := ar
SDK       := ../../sdk
ICSOS_ROOT:= ../..
SRC       ?= ../../../references/gcc-4.7.4
MPFR      ?= ../../../references/mpfr-3.0.1
MPCSRC    ?= ../../../references/mpc-1.0.1/src

STAGE     := /tmp/icsos-gcc
OBJ       := $(STAGE)/obj

CONF      := $(CURDIR)/config.h
CONFDIR   := $(CURDIR)

GMPDIR    := $(ICSOS_ROOT)/contrib/gmp
MPFRDIR   := $(ICSOS_ROOT)/contrib/mpfr
MPCDIR    := $(ICSOS_ROOT)/contrib/mpc
# cc1 compiles against the SDK-built overlays' headers so its mpz_*/mpn_* and
# mpfr_* references match the symbols libgmp.a / libmpfr.a actually export.
GMPINC    := -I$(GMPDIR) -I$(MPFRDIR)
# Link order matters for static archives: MPFR's objects reference GMP's
# internal __gmpn_* limb functions, so libmpfr.a must come BEFORE libgmp.a
# (dependency first, its dependency after) or the mpn refs stay undefined.
GMPMPFR   := $(MPFRDIR)/libmpfr.a $(GMPDIR)/libgmp.a

DEFS  := -DHAVE_CONFIG_H
INC   := -nostdinc -I$(SDK)/include -I$(CONFDIR) \
	-I$(SRC)/gcc -I$(SRC)/libcpp -I$(SRC)/libcpp/include -I$(SRC)/libiberty \
	$(GMPINC) -I$(MPFR)
# include/ holds GCC-internal build headers (filenames.h, ggc.h, ...) AND the
# target C headers (stdio.h, stddef.h, ...). It is added LAST so the SDK's
# libc headers (earlier) still win for <stdio.h> etc.; only names absent from
# the SDK (filenames.h, ...) resolve here.
INC   += -I$(SRC)/include
CFLAGS := $(APP_CFLAGS) $(DEFS) $(INC)

# GMP/MPFR static libs are linked into every final executable (cc1, gcc) so the
# constant-arithmetic calls resolve to the SDK-built implementations.
SDKLIBS := $(SDK)/tccsdk.c $(SDK)/posix.c $(SDK)/libtcc1.c $(SDK)/crt1.c $(SDK)/setjmp.c \
	$(GMPMPFR)

# ---- libcpp (the C preprocessor core; target-independent) ----
# 15 runtime files (makeucnid.c is a build-time code-gen tool, not linked in).
LIBCPP_C := \
	charset.c directives.c directives-only.c errors.c expr.c files.c \
	identifiers.c init.c lex.c line-map.c macro.c mkdeps.c pch.c \
	symtab.c traditional.c
LIBCPP_O := $(patsubst %.c,$(OBJ)/libcpp/%.o,$(LIBCPP_C))
LIBCPP_A := $(STAGE)/libcpp.a

$(OBJ)/libcpp/%.o: $(SRC)/libcpp/%.c $(CONF)
	@mkdir -p $(OBJ)/libcpp
	$(CC) $(CFLAGS) -c $< -o $@
$(LIBCPP_A): $(LIBCPP_O)
	$(AR) rcs $@ $^
libcpp: $(LIBCPP_A)
	@echo "libcpp: $(words $(LIBCPP_O)) objects -> $(LIBCPP_A)"

# ---- libiberty (the GNU common library; portable subset cc1 needs) ----
# Built as a static archive so the cc1 link pulls in only the members it
# references. Deliberately omitted: files duplicating SDK libc (memcpy, strcmp,
# strtod, ...) and files making OS calls the in-OS kernel does not serve
# (pexecute/pex-*/vfork/waitpid/tmpnam) -- process spawning is the gcc
# driver's job, not cc1's.
LIBIBERTY_C := \
	argv.c basename.c choose-temp.c concat.c copysign.c cplus-dem.c \
	crc32.c dyn-string.c ffs.c fibheap.c filename_cmp.c floatformat.c \
	hashtab.c hex.c insque.c lbasename.c make-relative-prefix.c \
	make-temp-file.c md5.c mempcpy.c objalloc.c obstack.c partition.c \
	splay-tree.c sort.c stack-limit.c stpcpy.c stpncpy.c strverscmp.c \
	unlink-if-ordinary.c xatexit.c xexit.c xmalloc.c xmemdup.c \
	xstrdup.c xstrerror.c xstrndup.c getopt.c getopt1.c \
	asprintf.c vasprintf.c cp-demangle.c safe-ctype.c \
	cp-demint.c physmem.c getruntime.c getpwd.c lrealpath.c
LIBIBERTY_O := $(patsubst %.c,$(OBJ)/libiberty/%.o,$(LIBIBERTY_C))
LIBIBERTY_A := $(STAGE)/libiberty.a

$(OBJ)/libiberty/%.o: $(SRC)/libiberty/%.c $(CONF)
	@mkdir -p $(OBJ)/libiberty
	$(CC) $(CFLAGS) -c $< -o $@
$(LIBIBERTY_A): $(LIBIBERTY_O)
	$(AR) rcs $@ $^
libiberty: $(LIBIBERTY_A)
	@echo "libiberty: $(words $(LIBIBERTY_O)) objects -> $(LIBIBERTY_A)"

# ---- libdecnumber (decimal floating point; pulled in by -ffloat-store/decimal) ----
# decNumber's dconfig.h pulls in tconfig.h (from the source dir) + config.h
# (our minimal little-endian x86-64 definition in decnuminc/). Built WITHOUT
# -fno-builtin / -fno-asynchronous-unwind-tables to match the proven host
# build; NOT -nostdinc so its <stdint.h>/<stddef.h> resolve to the SDK set.
DECNUMINC := $(CURDIR)/decnuminc
DECNUM_CFLAGS := -m64 -std=gnu89 -w -nostdlib -static -ffreestanding -fno-pie \
	-fno-pic -fno-stack-protector -fno-strict-aliasing -mcmodel=large \
	-mno-red-zone -I$(SRC)/libdecnumber -I$(SRC)/libdecnumber/bid \
	-I$(SRC)/libgcc -I$(DECNUMINC) -I$(SDK)/include $(DEFS)
LIBDECNUM_ROOT_C := decContext.c decNumber.c
LIBDECNUM_BID_C  := decimal32.c decimal64.c decimal128.c bid2dpd_dpd2bid.c \
	host-ieee32.c host-ieee64.c host-ieee128.c
LIBDECNUM_O := $(patsubst %.c,$(OBJ)/libdecnumber/%.o,$(LIBDECNUM_ROOT_C)) \
	$(patsubst %.c,$(OBJ)/libdecnumber/bid/%.o,$(LIBDECNUM_BID_C))
LIBDECNUM_A := $(STAGE)/libdecnumber.a

$(OBJ)/libdecnumber/%.o: $(SRC)/libdecnumber/%.c $(CONF)
	@mkdir -p $(dir $@)
	$(CC) $(DECNUM_CFLAGS) -c $< -o $@
$(OBJ)/libdecnumber/bid/%.o: $(SRC)/libdecnumber/bid/%.c $(CONF)
	@mkdir -p $(dir $@)
	$(CC) $(DECNUM_CFLAGS) -c $< -o $@
$(LIBDECNUM_A): $(LIBDECNUM_O)
	$(AR) rcs $@ $^
libdecnumber: $(LIBDECNUM_A)
	@echo "libdecnumber: $(words $(LIBDECNUM_O)) objects -> $(LIBDECNUM_A)"

# ---- libz (zlib: used by PCH compression) ----
# zlib's zconf.h pulls in host <sys/feature_tests.h>/<unistd.h>, so build it
# WITHOUT -nostdinc (host headers resolve normally); the SDK include set is not
# needed for zlib.
LIBZ_C := adler32.c compress.c crc32.c deflate.c infback.c inffast.c inflate.c \
	inftrees.c trees.c uncompr.c zutil.c
LIBZ_O := $(patsubst %.c,$(OBJ)/libz/%.o,$(LIBZ_C))
LIBZ_A := $(STAGE)/libz.a

$(OBJ)/libz/%.o: $(SRC)/zlib/%.c
	@mkdir -p $(OBJ)/libz
	$(CC) $(APP_CFLAGS) -I$(SRC)/zlib -c $< -o $@
$(LIBZ_A): $(LIBZ_O)
	$(AR) rcs $@ $^
libz: $(LIBZ_A)
	@echo "libz: $(words $(LIBZ_O)) objects -> $(LIBZ_A)"

# ---- GMP / MPFR / MPC (SDK-built overlays; each has its own Makefile) ----
# Each overlay builds its .a from the references/ source tree against the SDK.
# The .a is a build product: these rules regenerate it from source only when it
# is missing. Building `all` explicitly keeps this robust to each overlay's
# internal target ordering (GMP's object rules used to shadow `all` as the
# default goal; see the .DEFAULT_GOAL pin in contrib/gmp/Makefile).
$(GMPDIR)/libgmp.a:
	$(MAKE) -C $(GMPDIR) all
$(MPFRDIR)/libmpfr.a:
	$(MAKE) -C $(MPFRDIR) all
$(MPCDIR)/libmpc.a:
	$(MAKE) -C $(MPCDIR) all

# Build all three SDK math libraries from source (regenerates any that are
# missing). Standalone: `make -C contrib/gcc mathlibs`.
MATHLIBS := $(GMPDIR)/libgmp.a $(MPFRDIR)/libmpfr.a $(MPCDIR)/libmpc.a
mathlibs: $(MATHLIBS)
	@echo "mathlibs: GMP/MPFR/MPC built from source"

# ---- cc1 (C frontend + middle end + config/i386 target) ----
GEN         := $(CURDIR)/gen
SHIMSDIR    := $(CURDIR)/shims
CC1_OBJDIR  := $(STAGE)/cc1objs
CC1_OBJLIST := $(CURDIR)/cc1-objs.txt
# 349 objects: flat gcc/*.o + c-family/*.o + common/config/i386/*.o + the three
# hand-written shims. The list is committed (cc1-objs.txt) so the build is
# reproducible without re-deriving it.
CC1_O    := $(shell tr '\n' ' ' < $(CC1_OBJLIST))
# cc1-objs.txt entries already carry the .o suffix, so just prefix the dir.
CC1_OBJS := $(addprefix $(CC1_OBJDIR)/,$(CC1_O))

# DATESTAMP is fixed (not `$(shell date)`) so the build is reproducible; it
# only feeds the __DATE__ string in version.c. The values are passed as C
# string literals: the backslash-escaped quotes survive the single shell
# interpretation of the recipe (the unquoted $(VERDEFS) in compile_one.sh
# relied on the quotes already living inside a bash variable, which is not the
# case for a make variable expanded onto the command line).
VERDEFS := -DBASEVER=\"4.7.4\" -DBUGURL=\"\" -DDATESTAMP=\"20260830\" -DDEVPHASE=\"\" -DREVISION=\"\" -DPKGVERSION=\"4.7.4\" -DTARGET_NAME=\"x86_64-ics-os\"
GASDEFS := -DHAVE_GAS_CFI_PERSONALITY_DIRECTIVE=1 -DHAVE_GAS_CFI_DIRECTIVE=1 -DHAVE_COMDAT_GROUP=1 -DHAVE_GAS_SHF_MERGE=1 -DHAVE_GAS_CFI_SECTIONS_DIRECTIVE=1 -DHAVE_GAS_HIDDEN=1 -DHAVE_GAS_MAX_SKIP_P2ALIGN=65535 -DHAVE_AS_GOTOFF_IN_DATA=1 -DHAVE_AS_IX86_CMOV_SUN_SYNTAX=1 -DHAVE_AS_IX86_FFREEP=1 -DHAVE_AS_IX86_FILDQ=1 -DHAVE_AS_IX86_FILDS=1 -DHAVE_AS_IX86_REP_LOCK_PREFIX=1 -DHAVE_AS_TLS=1 -DHAVE_AS_GOTTPLTPCALL=1 -DHAVE_AS_TLSDIRECT=1 -DHAVE_AS_CFI_SECTIONS=1 -DHAVE_AS_X86_CMPXCHG16B=1
# Generated headers (gen/) come FIRST; SDK libc headers come before GCC's
# include/ so <stdio.h> etc. still resolve to the SDK set.
CC1_INC := -I$(GEN) -I$(CONFDIR) -I$(SDK)/include \
	-I$(SRC)/gcc -I$(SRC)/gcc/config -I$(SRC)/gcc/config/i386 \
	-I$(SRC)/libcpp -I$(SRC)/libcpp/include -I$(SRC)/libiberty \
	-I$(GMPDIR) -I$(MPFRDIR) -I$(MPFR) -I$(MPCSRC) \
	-I$(SRC)/libdecnumber -I$(SRC)/libdecnumber/bid -I$(SRC)/libgcc -I$(SRC)/include
CC1_CFLAGS := $(APP_CFLAGS) -DIN_GCC $(DEFS) $(VERDEFS) $(GASDEFS) $(CC1_INC)

# Source resolution order (first existing wins): generated -> shims -> gcc/ ->
# config/i386/ -> config/. This matches the proven compile_one.sh, extended
# with shims/ and gcc/config/ (the original four-candidate list covered neither
# -- default-c.c lives in gcc/config/).
$(CC1_OBJDIR)/%.o: $(CONF)
	@mkdir -p $(dir $@)
	@src=$$(for cand in $(GEN)/$*.c $(SHIMSDIR)/$*.c $(SRC)/gcc/$*.c $(SRC)/gcc/config/i386/$*.c $(SRC)/gcc/config/$*.c; do if [ -f "$$cand" ]; then printf '%s' "$$cand"; break; fi; done); \
	if [ -z "$$src" ]; then echo "CC1_MISSING_SRC $*.o"; exit 1; fi; \
	$(CC) $(CC1_CFLAGS) -c "$$src" -o $@ 2>$(CC1_OBJDIR)/$*.err || { echo "CC1_OBJ_FAIL $*.o"; sed -n '1,40p' $(CC1_OBJDIR)/$*.err; exit 1; }

CC1         := $(STAGE)/cc1
CC1_CLINK   := $(APP_CFLAGS) -nostdinc -I$(SDK)/include -I$(CONFDIR) $(DEFS)
CC1_LDFLAGS := $(APP_LDFLAGS)
CC1_SDKC    := $(SDK)/tccsdk.c $(SDK)/posix.c $(SDK)/libtcc1.c $(SDK)/crt1.c $(SDK)/setjmp.c
# Static-archive link order: cc1 objects, then the SDK .c files, then the
# component libs. libmpfr.a must precede libgmp.a (its mpn_* deps); the
# --start-group/--end-group lets mutual references among the component libs
# resolve regardless of order.
CC1_LIBS := $(LIBIBERTY_A) $(LIBCPP_A) $(LIBDECNUM_A) $(LIBZ_A) \
	$(MPCDIR)/libmpc.a $(MPFRDIR)/libmpfr.a $(GMPDIR)/libgmp.a

cc1: $(CC1)
$(CC1): $(CC1_OBJS) $(CC1_SDKC) $(CC1_LIBS)
	$(CC) $(CC1_CLINK) $(CC1_LDFLAGS) $(CC1_OBJS) $(CC1_SDKC) \
		-Wl,--start-group $(CC1_LIBS) -Wl,--end-group \
		-o $@ -Wl,--no-undefined
	@echo "cc1: linked OK -> $@ ($$(stat -c%s $@) bytes)"

# ---- GMP/MPFR wiring regression test ----
# A tiny program that calls both GMP and MPFR, built with the cc1 flags and
# linked against the SDK + the SDK-built GMP/MPFR libs. Fails at link time
# (-Wl,--no-undefined) if the overlay's GMP/MPFR header-to-symbol wiring is
# wrong (e.g. cc1 pointed at the host gmp.h instead of contrib/gmp's gmp.h).
test-gmpmpfr: test_gmpmpfr.c $(GMPMPFR)
	$(CC) $(CFLAGS) $(APP_LDFLAGS) $< $(SDKLIBS) -o $@ -Wl,--no-undefined
	@echo "GCC_GMPMPFR_WIRING_OK"

.PHONY: all libcpp libiberty libdecnumber libz cc1 mathlibs test-gmpmpfr clean

all: cc1

clean:
	rm -rf $(STAGE)
	rm -f test-gmpmpfr
