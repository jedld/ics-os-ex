# GNU Make 3.82 native ICS-OS rebuild.  This is intentionally usable by both
# the host make and the in-OS make port; callers only override tool paths.
CC ?= gcc
LD ?= ld
TOOLPREFIX ?=
ROOT ?= /work/makesrc
SRC ?= $(ROOT)/src
SDK ?= $(ROOT)/sdk
OUT ?= /work/makeobj
BINDIR ?= /work

CFLAGS = -m64 -std=gnu89 -w -nostdlib -fno-builtin -static -ffreestanding \
	-fno-pie -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables \
	-fno-unwind-tables -fno-strict-aliasing -mcmodel=large -mno-red-zone \
	-nostdinc -I$(SDK)/include -I$(SRC) -I$(SRC)/glob -DHAVE_CONFIG_H
LDSCRIPT ?= /icsos/apps/ldscripts/elf_x86_64.xc
LDFLAGS = -m elf_x86_64 -T $(LDSCRIPT) --build-id=none -z noexecstack -nostdlib

MAKE_NAMES = ar arscan commands default dir expand file function getopt getopt1 \
	implicit job main misc read remake rule signame strcache variable version \
	vpath hash remote-stub
MAKE_OBJS = $(addprefix $(OUT)/,$(addsuffix .o,$(MAKE_NAMES))) \
	$(OUT)/glob.o $(OUT)/fnmatch.o
SDK_NAMES = tccsdk posix libtcc1 crt1 setjmp
SDK_OBJS = $(addprefix $(OUT)/sdk-,$(addsuffix .o,$(SDK_NAMES)))

all: $(BINDIR)/make.exe

$(OUT)/%.o: $(SRC)/%.c
	$(CC) $(TOOLPREFIX) $(CFLAGS) -c $< -o $@

$(OUT)/glob.o: $(SRC)/glob/glob.c
	$(CC) $(TOOLPREFIX) $(CFLAGS) -c $< -o $@

$(OUT)/fnmatch.o: $(SRC)/glob/fnmatch.c
	$(CC) $(TOOLPREFIX) $(CFLAGS) -c $< -o $@

$(OUT)/sdk-%.o: $(SDK)/%.c
	$(CC) $(TOOLPREFIX) $(CFLAGS) -c $< -o $@

$(BINDIR)/make.exe: $(MAKE_OBJS) $(SDK_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(MAKE_OBJS) $(SDK_OBJS)

.PHONY: all
