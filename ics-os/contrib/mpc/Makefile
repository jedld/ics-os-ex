# MPC 1.0.1 overlay — builds libmpc.a (multiple-precision complex) for the
# ICS-OS GCC self-host toolchain, using the in-OS SDK (freestanding).
#
# MPC 1.0.1 keeps its library sources in src/ (unlike MPFR's flat layout).
# mpc-impl.h does an unguarded #include "config.h", so we ship a minimal
# config.h in CONFDIR (contrib/mpc) defining only the HAVE_* macros the SDK
# satisfies; optional complex.h / locale.h / dlfcn.h paths stay off.
#
# gmp.h comes from the GMP overlay (contrib/gmp, the __gmp*/__gmpn* symbol
# mapping); mpfr.h comes from the MPFR source tree. Both are public-API
# headers, so no -DHAVE_CONFIG_H is needed.

CONFDIR  := $(CURDIR)
SRC      := /home/jedld/workspace/ics-os/references/mpc-1.0.1
SDKDIR   := $(CURDIR)/../../sdk
SDK_INC  := $(SDKDIR)/include
GMP_INC  := $(CURDIR)/../gmp
MPFR_INC := /home/jedld/workspace/ics-os/references/mpfr-3.0.1

OBJ := obj
LIB := libmpc.a

include $(SDKDIR)/app.mk

HOSTCC ?= gcc

# -nostdinc forces every <...> include to resolve against the SDK headers
# (the real in-OS symbol surface) instead of host glibc, mirroring the GMP /
# MPFR overlays. CONFDIR supplies our config.h; $(SRC)/src supplies mpc.h /
# mpc-impl.h / mpc-log.h; GMP_INC / MPFR_INC supply the public gmp.h / mpfr.h.
MPCINC   := -nostdinc -I$(SDK_INC) -I$(CONFDIR) -I$(SRC)/src -I$(GMP_INC) -I$(MPFR_INC)
MPC_CFLAGS := $(APP_CFLAGS) $(MPCINC)

# logging.c only generates mpc_log_* wrappers that dlsym() the real mpc_*
# symbols at runtime; they are only referenced when a translation unit includes
# mpc-log.h (in place of mpc.h). Our toolchain always uses mpc.h, so logging.c
# is dead weight and would drag in an unresolvable dlsym — exclude it.
MPC_EXCLUDE := logging.c
MPC_C  := $(filter-out $(addprefix $(SRC)/src/,$(MPC_EXCLUDE)),\
           $(wildcard $(SRC)/src/*.c))
OBJS   := $(patsubst $(SRC)/src/%.c,$(OBJ)/%.o,$(MPC_C))

.PHONY: all libmpc test-mpc clean

all: libmpc

libmpc: $(LIB)

# MPC's symbols depend on MPFR, which depends on GMP; the archive holds only
# MPC objects — the consumer links libmpc.a, then libmpfr.a, then libgmp.a.
$(LIB): $(OBJS)
	ar rcs $@ $(OBJS)

$(OBJ)/%.o: $(SRC)/src/%.c | $(OBJ)
	$(CC) $(MPC_CFLAGS) -c -o $@ $<

$(OBJ):
	mkdir -p $@

# Host-side functional test: link a small program against our libmpc.a +
# libmpfr.a + libgmp.a with the HOST libc and run it. Exercises the public
# mpc_add / mpc_mul / mpc_powi / mpc_sqrt API cc1 depends on.
test-mpc: $(LIB) test_mpc_host
	./test_mpc_host

test_mpc_host: test_mpc.c $(LIB) ../mpfr/libmpfr.a $(GMP_INC)/libgmp.a
	$(HOSTCC) -m64 -no-pie -I$(GMP_INC) -I$(MPFR_INC) -I$(SRC)/src -I$(CONFDIR) \
		test_mpc.c $(LIB) ../mpfr/libmpfr.a $(GMP_INC)/libgmp.a -o $@

clean:
	rm -rf $(OBJ) $(LIB) test_mpc_host
