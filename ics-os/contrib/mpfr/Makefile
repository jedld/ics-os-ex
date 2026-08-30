# MPFR 3.0.1 overlay — builds libmpfr.a (multiple-precision floating point)
# for the ICS-OS GCC self-host toolchain, using the in-OS SDK (freestanding).
#
# MPFR 3.0.1 is a top-level-source autoconf project: all 217 library .c files
# live at the repo root (no src/ dir). It takes its config from command-line
# -D defines (DEFS), NOT a config.h — the #ifdef HAVE_CONFIG_H guards in
# clears.c/inits.c/inits2.c/printf.c/vasprintf.c stay false. mparam.h is a
# BUILT_SOURCES file generated from mparam_h.in by configure; we ship the
# configure-generated copy (tuned for x86-64) so the SDK build needs no
# autotools.
#
# gmp.h comes from the GMP overlay (contrib/gmp) — the __gmp*/__gmpn* symbol
# mapping. mpfr-impl.h includes "gmp.h" (public GMP API) before "mpfr.h".
#
# The host build is the gap detector for what the SDK lacks (libm growth).

CONFDIR := $(CURDIR)
SRC     := /home/jedld/workspace/ics-os/references/mpfr-3.0.1
SDKDIR  := $(CURDIR)/../../sdk
SDK_INC := $(SDKDIR)/include
GMPDIR  := $(CURDIR)/../gmp
GMP_INC := $(GMPDIR)

OBJ     := obj
LIB     := libmpfr.a

include $(SDKDIR)/app.mk

HOSTCC ?= gcc

# MPFR 3.0.1 config: the subset of the host ./configure DEFS that the SDK
# satisfies (ceil/floor/fabs/ldexp, inttypes/stdint/wchar/stdarg, intmax_t,
# va_copy, attribute mode, and the x86-64 CPU facts HAVE_DENORMS +
# HAVE_LDOUBLE_IEEE_EXT_LITTLE). Dropped because the SDK lacks the function/
# header and MPFR has #ifdef fallbacks for each: HAVE_LOCALE_H, HAVE_SETLOCALE,
# HAVE_GETTIMEOFDAY, HAVE_SYS_TIME_H, TIME_WITH_SYS_TIME, MPFR_HAVE_FESETROUND,
# HAVE_ROUND, HAVE_TRUNC, HAVE_NEARBYINT, HAVE_ALLOCA_H (MPFR never calls
# alloca), and libtool's LT_OBJDIR.
MPFR_DEFS := \
  -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_WCHAR_H=1 -DHAVE_STDARG=1 \
  -DHAVE_LONG_LONG=1 -DHAVE_INTMAX_T=1 -DMPFR_HAVE_INTMAX_MAX=1 \
  -DHAVE_DENORMS=1 -DHAVE_FLOOR=1 -DHAVE_CEIL=1 \
  -DHAVE_LDOUBLE_IEEE_EXT_LITTLE=1 -DHAVE_VA_COPY=1 -DHAVE_ATTRIBUTE_MODE=1

# -nostdinc forces every #include <...> to resolve against the SDK headers
# (the real in-OS symbol surface) instead of host glibc, mirroring the GMP
# overlay. Without it, glibc's ctype.h macros would leak __ctype_b_loc.
MPFRINC   := -nostdinc -I$(SDK_INC) -I$(CONFDIR) -I$(SRC) -I$(GMP_INC)
MPFR_CFLAGS := $(APP_CFLAGS) $(MPFR_DEFS) $(MPFRINC)

# The 217 library sources = every top-level .c except the 5 non-library ones:
# ansi2knr.c (K&R converter tool), speed.c/tuneup.c (standalone benchmark
# programs), and jyn_asympt.c / round_raw_generic.c (which are #include'd by
# jn.c/yn.c and round_prec.c respectively, listed in EXTRA_DIST).
MPFR_EXCLUDE := ansi2knr.c speed.c tuneup.c jyn_asympt.c round_raw_generic.c
MPFR_C := $(filter-out $(addprefix $(SRC)/,$(MPFR_EXCLUDE)),\
           $(wildcard $(SRC)/*.c))

OBJS := $(patsubst $(SRC)/%.c,$(OBJ)/%.o,$(MPFR_C))

.PHONY: all libmpfr test-mpfr clean

all: libmpfr

libmpfr: $(LIB)

# MPFR's symbols depend on GMP; pull libgmp.a in so the archive is complete.
$(LIB): $(OBJS)
	ar rcs $@ $(OBJS)

$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	$(CC) $(MPFR_CFLAGS) -c -o $@ $<

$(OBJ):
	mkdir -p $@

# Host functional test: includes <gmp.h> before <mpfr.h>, links our
# libmpfr.a + libgmp.a against host libc.
test-mpfr: $(LIB) test_mpfr_host
	./test_mpfr_host

test_mpfr_host: test_mpfr.c $(LIB) $(GMPDIR)/libgmp.a
	$(HOSTCC) -m64 -no-pie -I$(GMP_INC) -I$(SRC) -I$(CONFDIR) test_mpfr.c $(LIB) $(GMPDIR)/libgmp.a -o $@

clean:
	rm -rf $(OBJ) $(LIB) test_mpfr_host
