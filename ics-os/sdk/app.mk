# Common flags for ICS-OS user-space programs built on a modern host gcc.
# 32-bit, no PIE (Ubuntu 16.10+ defaults to PIE), no stack protector, no SSE.
APP_CFLAGS ?= -m32 -std=gnu89 -w -nostdlib -fno-builtin -static -ffreestanding \
	-fno-pie -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables \
	-mno-mmx -mno-sse -mno-sse2 -msoft-float -fno-strict-aliasing
APP_LDFLAGS ?= -m32 -no-pie -nostdlib -static
