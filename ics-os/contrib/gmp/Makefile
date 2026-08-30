# GMP 5.1.3 — host-gcc build against the ICS-OS SDK, producing libgmp.a
# (x86-64 objects with the SDK/int-0x30 ABI) for linking into the in-OS cc1.
# Same overlay pattern as contrib/binutils and contrib/gcc. Uses the portable
# mpn/generic implementations (no target asm / gas support needed); adequate
# for a bootstrap compiler.
#
# Generated headers (fib_table.h, fac_table.h, mp_bases.h, jacobitab.h,
# trialdivtab.h, perfsqr.h) + table .c files (mpn_fib_table.c,
# mpn_bases_table.c) are produced once by GMP's own generators (gen-fib /
# gen-fac / gen-bases / gen-jacobitab / gen-trialdivtab / gen-psqr) run
# against the host, and live here in CONFDIR so gmp-impl.h's / the array
# initializers' #include of them resolves via -I$(CONFDIR).
#
# Some mpn/generic sources are compiled once per OPERATION_* (logops_n,
# popham, sb_div_sec, sbpi1_div_sec); they are pulled out of the wildcard and
# given explicit per-operation rules below.
#
#   make libgmp     # builds $(OBJ)/... and libgmp.a
#   make clean

SRC     ?= ../../../references/gmp-5.1.3
SDK     := ../../sdk
CONFDIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
OBJ     ?= $(CONFDIR)/obj
CC      := gcc
AR      := ar
HOSTCC  ?= gcc

include $(SDK)/app.mk

GMPDEFS  := -DHAVE_CONFIG_H -DVERSION='"5.1.3"'
# SDK first (libc), then CONFDIR (generated gmp.h/config.h/tables), then the
# GMP source dir (gmp-impl.h, longlong.h). CONFDIR precedes $(SRC) so our
# generated gmp.h/config.h win.
GMPINC   := -nostdinc -I$(SDK)/include -I$(CONFDIR) -I$(SRC)
GMPFLAGS := $(APP_CFLAGS) $(GMPDEFS) $(GMPINC)

# Portable library sources. Excludes the gen-* code generators, bootstrap.c,
# and the test/debug thread-allocation variants (keep only tal-notreent.c).
TOP_C := memory.c version.c errno.c extract-dbl.c invalid.c compat.c \
	assert.c nextprime.c primesieve.c mp_bpl.c mp_clz_tab.c mp_dv_tab.c \
	mp_get_fns.c mp_minv_tab.c mp_set_fns.c tal-notreent.c

# generated table data (compiled from CONFDIR)
TABLE_C := $(CONFDIR)/mpn_fib_table.c $(CONFDIR)/mpn_bases_table.c
TABLE_O := $(OBJ)/mpn_fib_table.o $(OBJ)/mpn_bases_table.o

# ---- multi-compiled generic files (one object per OPERATION_*) ----
DUAL_SRCS := logops_n.c popham.c sb_div_sec.c sbpi1_div_sec.c
DUAL_C := $(addprefix $(SRC)/mpn/generic/,$(DUAL_SRCS))
GENERIC_C := $(filter-out $(DUAL_C),$(wildcard $(SRC)/mpn/generic/*.c))
SUB_C := $(wildcard $(SRC)/mpz/*.c) $(wildcard $(SRC)/mpq/*.c) \
	$(wildcard $(SRC)/mpf/*.c) $(wildcard $(SRC)/rand/*.c) $(GENERIC_C)
SRCTREE_C := $(addprefix $(SRC)/,$(TOP_C)) $(SUB_C)

LOGOPS_N_OPS := and_n andn_n nand_n ior_n iorn_n nior_n xor_n xnor_n
POPHAM_OPS   := popcount hamdist
SB_DIV_OPS   := sb_div_qr_sec sb_div_r_sec
SBPI1_DIV_OPS:= sbpi1_div_qr_sec sbpi1_div_r_sec

DUAL_O := $(addsuffix .o,$(addprefix $(OBJ)/logops_n-,$(LOGOPS_N_OPS))) \
	$(addsuffix .o,$(addprefix $(OBJ)/popham-,$(POPHAM_OPS))) \
	$(addsuffix .o,$(addprefix $(OBJ)/sb_div_sec-,$(SB_DIV_OPS))) \
	$(addsuffix .o,$(addprefix $(OBJ)/sbpi1_div_sec-,$(SBPI1_DIV_OPS)))

define dual_rule
$(OBJ)/$(1)-$(2).o: $(SRC)/mpn/generic/$(1).c | $(OBJ)
	$(CC) $(GMPFLAGS) -DOPERATION_$(2)=1 -c $$< -o $$@
endef
$(foreach op,$(LOGOPS_N_OPS),  $(eval $(call dual_rule,logops_n,$(op))))
$(foreach op,$(POPHAM_OPS),    $(eval $(call dual_rule,popham,$(op))))
$(foreach op,$(SB_DIV_OPS),    $(eval $(call dual_rule,sb_div_sec,$(op))))
$(foreach op,$(SBPI1_DIV_OPS), $(eval $(call dual_rule,sbpi1_div_sec,$(op))))

# Map each source to a flat, unique object path: the relative path's '/' chars
# become '_', so e.g. mpn/generic/add.c -> obj/mpn_generic_add.o. This avoids
# `ar` basename collisions (mpz/add.o vs mpf/add.o vs mpn/generic/add.o would
# otherwise all be stored as member "add.o" and clobber each other).
SRC2OBJ = $(addprefix $(OBJ)/,$(subst /,_,$(patsubst $(SRC)/%.c,%.o,$(1))))
ALL_O := $(foreach s,$(SRCTREE_C),$(call SRC2OBJ,$(s))) $(TABLE_O) $(DUAL_O)

define src_rule
$(call SRC2OBJ,$(1)): $(1) | $(OBJ)
	$(CC) $(GMPFLAGS) -c $$< -o $$@
endef
$(foreach s,$(SRCTREE_C),$(eval $(call src_rule,$(s))))

$(OBJ):
	mkdir -p $(OBJ)

# The dual_rule/src_rule $(foreach $(eval ...)) blocks above create the object
# targets BEFORE `all`, so the first object would otherwise become make's
# default goal. Pin the default to `all` so a bare `make` (as the gcc overlay's
# fallback rule invokes it) builds the whole library, not a single object.
.DEFAULT_GOAL := all

.PHONY: all libgmp test-gmp clean
all: libgmp
libgmp: libgmp.a

libgmp.a: $(ALL_O)
	$(AR) rcs $@ $^

$(OBJ)/mpn_fib_table.o: $(CONFDIR)/mpn_fib_table.c | $(OBJ)
	$(CC) $(GMPFLAGS) -c $< -o $@
$(OBJ)/mpn_bases_table.o: $(CONFDIR)/mpn_bases_table.c | $(OBJ)
	$(CC) $(GMPFLAGS) -c $< -o $@

# Host-side functional test: link test_gmp.c against libgmp.a with the HOST
# libc (for printf) and run it on the host. Exercises the public mpz/mpf/mpq/
# rand API plus the internal mpn_* routines cc1 depends on.
test-gmp: libgmp.a test_gmp_host
	./test_gmp_host

test_gmp_host: test_gmp.c libgmp.a
	$(HOSTCC) -m64 -no-pie -I$(CONFDIR) test_gmp.c libgmp.a -o $@

clean:
	rm -rf $(OBJ) libgmp.a test_gmp_host
