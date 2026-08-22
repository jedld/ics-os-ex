#!/bin/bash
# Build a FAT16-friendly (8.3) TinyCC source tree for in-OS tccboot.
# Long names like x86_64-gen.c are renamed; #include lines are patched.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$ROOT/contrib/tcc"
DST="${1:-/tmp/icsos-tcc-short}"

rm -rf "$DST"
mkdir -p "$DST"

# Files that already fit in 8.3, plus renamed long ones.
cp -a "$SRC/tcc.c" "$SRC/tcc.h" "$SRC/config.h" \
      "$SRC/libtcc.c" "$SRC/libtcc.h" \
      "$SRC/tccpp.c" "$SRC/tccgen.c" "$SRC/tccelf.c" \
      "$SRC/tccrun.c" "$SRC/tccasm.c" "$SRC/tcctools.c" \
      "$SRC/tcctok.h" "$SRC/stab.h" "$SRC/stab.def" "$SRC/elf.h" \
      "$DST/"

cp -a "$SRC/x86_64-gen.c"  "$DST/x64gen.c"
cp -a "$SRC/x86_64-link.c" "$DST/x64lnk.c"
cp -a "$SRC/x86_64-asm.h"  "$DST/x64asm.h"
cp -a "$SRC/i386-asm.c"    "$DST/i386asm.c"
cp -a "$SRC/i386-asm.h"    "$DST/i386asm.h"
cp -a "$SRC/i386-tok.h"    "$DST/i386tok.h"

# Patch includes in the short tree only.
for f in "$DST"/*; do
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

echo "staged 8.3 TinyCC sources under $DST ($(ls -1 "$DST" | wc -l) files)"
