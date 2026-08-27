#!/bin/bash
# Pack an 8.3 TinyCC tree + SDK + tcc.exe into one directory for tccsrc.tar.
# Everything is ramdisk-staged from a single ISO tar map (no ATAPI copies).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$ROOT/contrib/tcc"
DST="${1:-/tmp/icsos-tcc-short}"

rm -rf "$DST"
mkdir -p "$DST/tcc" "$DST/sdk"

cp -a "$SRC/tcc.c" "$SRC/tcc.h" "$SRC/config.h" \
      "$SRC/libtcc.c" "$SRC/libtcc.h" \
      "$SRC/tccpp.c" "$SRC/tccgen.c" "$SRC/tccelf.c" \
      "$SRC/tccrun.c" "$SRC/tccasm.c" "$SRC/tcctools.c" \
      "$SRC/tcctok.h" "$SRC/stab.h" "$SRC/stab.def" "$SRC/elf.h" \
      "$DST/tcc/"

cp -a "$SRC/x86_64-gen.c"  "$DST/tcc/x64gen.c"
cp -a "$SRC/x86_64-link.c" "$DST/tcc/x64lnk.c"
cp -a "$SRC/x86_64-asm.h"  "$DST/tcc/x64asm.h"
cp -a "$SRC/i386-asm.c"    "$DST/tcc/i386asm.c"
cp -a "$SRC/i386-asm.h"    "$DST/tcc/i386asm.h"
cp -a "$SRC/i386-tok.h"    "$DST/tcc/i386tok.h"

for f in "$DST/tcc/"*; do
  [ -f "$f" ] || continue
  sed -i \
    -e 's/"x86_64-gen\.c"/"x64gen.c"/g' \
    -e 's/"x86_64-link\.c"/"x64lnk.c"/g' \
    -e 's/"x86_64-asm\.h"/"x64asm.h"/g' \
    -e 's/"i386-asm\.c"/"i386asm.c"/g' \
    -e 's/"i386-asm\.h"/"i386asm.h"/g' \
    -e 's/"i386-tok\.h"/"i386tok.h"/g' \
    "$f"
done

# SDK runtime is a FIXED dependency (like libc): the in-OS tccboot links the
# host-prebuilt objects instead of parsing the raw SDK sources in-OS. Parsing
# them in-OS hit TinyCC/SDK-header clashes (va_list, size_t) that the host
# gcc handles fine. Build the 5 runtime objects with host gcc (APP_CFLAGS).
SDKOBJ_CFLAGS="-m64 -std=gnu89 -w -nostdlib -fno-builtin -static -ffreestanding \
  -fno-pie -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables \
  -fno-unwind-tables -fno-stack-clash-protection \
  -fno-strict-aliasing -mcmodel=large \
  -mno-red-zone -nostdinc -I$ROOT/sdk/include -I$ROOT/contrib/tcc \
  -DTCC_TARGET_X86_64 -DONE_SOURCE=0 -DCONFIG_TCC_STATIC"
mkdir -p "$DST/sdkobj"
for f in tccsdk posix libtcc1 crt1 setjmp; do
  echo "host: building SDK runtime object $f.o"
  gcc -c $SDKOBJ_CFLAGS -o "$DST/sdkobj/$f.o" "$ROOT/sdk/$f.c"
done
# Keep the raw SDK sources + header for reference / future in-OS compiles.
cp -a "$ROOT/sdk/tccsdk.c" "$ROOT/sdk/posix.c" "$ROOT/sdk/libtcc1.c" \
      "$ROOT/sdk/crt1.c" "$ROOT/sdk/setjmp.c" "$DST/sdk/"
cp -a "$ROOT/sdk/dexsdk.h" "$DST/sdk/dexsdk.h"
cp -a "$ROOT/apps/tcc.exe" "$DST/tcc.exe"
cp -a "$ROOT/contrib/selfhost/min.c" "$DST/min.c"
cp -a "$ROOT/contrib/selfhost/args.c" "$DST/args.c"

# Host-preprocess so in-OS tcc does not open headers (ISO/FAT include I/O GPFs).
# tcc -E defines __TINYC__, so sdk/stdarg.h expands to struct va_list.
# Drop #line markers (they confuse in-OS tcc and leak host paths).
mkdir -p "$DST/pre"
for f in tcc.c libtcc.c tccpp.c tccgen.c tccelf.c tccrun.c tccasm.c \
         x64gen.c x64lnk.c i386asm.c; do
  tcc -E -DONE_SOURCE=0 -DTCC_TARGET_X86_64 -DCONFIG_TCC_STATIC \
    -I"$DST/tcc" -I"$ROOT/sdk/include" "$DST/tcc/$f" \
    | sed '/^#/d' > "$DST/pre/$f"
done

echo "staged tccboot tree under $DST (tcc=$(ls -1 "$DST/tcc" | wc -l) pre=$(ls -1 "$DST/pre" | wc -l))"
