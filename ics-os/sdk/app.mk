# Common flags for ICS-OS user-space programs (x86_64 long mode).
SDK ?= ../../sdk
APP_CFLAGS ?= -m64 -std=gnu89 -w -nostdlib -fno-builtin -static -ffreestanding \
	-fno-pie -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables \
	-fno-strict-aliasing -mcmodel=large -mno-red-zone
APP_LDFLAGS ?= -m64 -no-pie -nostdlib -static -Wl,--gc-sections
APP_LIBS ?= $(SDK)/tccsdk.c $(SDK)/libtcc1.c $(SDK)/crt1.c
