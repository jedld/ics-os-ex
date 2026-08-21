include ../../sdk/app.mk

# Build TinyCC as an ICS-OS ELF executable using the host gcc + dexsdk.
NAME=tcc
EXE=$(NAME).exe
ICSOS_ROOT=../..
SDK=$(ICSOS_ROOT)/sdk
TCC_SRC=.

override CFLAGS := -m32 -std=gnu89 -w -nostdlib -fno-builtin -static -ffreestanding \
	-fno-pie -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables \
	-fno-strict-aliasing -mhard-float -mfpmath=387 \
	-mno-mmx -mno-sse -mno-sse2 \
	-nostdinc -I$(SDK)/include -I$(TCC_SRC) \
	-DTCC_TARGET_I386 -DONE_SOURCE -DCONFIG_TCC_STATIC
LIBS := $(SDK)/tccsdk.c $(SDK)/posix.c $(SDK)/libtcc1.c $(SDK)/crt1.c $(SDK)/setjmp.c

$(EXE): tcc.c tcc.h config.h libtcc.c $(SDK)/posix.c $(SDK)/tccsdk.c $(SDK)/crt1.c $(SDK)/setjmp.c
	$(CC) $(CFLAGS) $(APP_LDFLAGS) -Wl,-s -o$(EXE) tcc.c $(LIBS)

install: $(EXE)
	mkdir -p $(ICSOS_ROOT)/apps
	cp $(EXE) $(ICSOS_ROOT)/apps

clean:
	rm -f $(EXE)

.PHONY: install clean
