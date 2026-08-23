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

cp -a "$ROOT/sdk/tccsdk.c" "$ROOT/sdk/posix.c" "$ROOT/sdk/libtcc1.c" \
      "$ROOT/sdk/crt1.c" "$ROOT/sdk/setjmp.c" "$DST/sdk/"
# The tccboot link step compiles these SDK sources raw (not preprocessed),
# so the quoted #include "dexsdk.h" in tccsdk.c/crt1.c must resolve.
cp -a "$ROOT/sdk/dexsdk.h" "$DST/sdk/dexsdk.h"
cp -a "$ROOT/apps/tcc.exe" "$DST/tcc.exe"
cp -a "$ROOT/contrib/selfhost/min.c" "$DST/min.c"

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
