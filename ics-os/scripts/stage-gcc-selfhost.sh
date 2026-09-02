#!/bin/bash
# Stage the exact GCC 4.7.4 C-frontend sources and headers needed by the
# strict in-OS compiler-closure test. Sources remain read-only on ISO9660;
# generated objects and executables are written to the FAT /work disk.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OS="$ROOT/ics-os"
REF="$ROOT/references"
DST="${1:-/tmp/icsos-gcc-selfhost-src}"
UP="$DST/gccsrc/up"

rm -rf "$DST"
mkdir -p "$UP/gcc" "$UP/libcpp" "$UP/libiberty" "$UP/libdecnumber" \
  "$UP/zlib" "$UP/include" "$DST/gccsrc/gen" "$DST/gccsrc/shims" \
  "$DST/gccsrc/units" \
  "$UP/mpfr" "$UP/mpc/src" \
  "$DST/gccsrc/sdk" "$DST/gccsrc/conf/gcc" \
  "$DST/gccsrc/conf/decnumber" "$DST/gccsrc/conf/gmp" \
  "$DST/gccsrc/conf/mpfr" "$DST/gccsrc/conf/mpc" "$DST/seed"

# GCC sources, headers, machine descriptions and option definitions are
# compiler input. Some selected translation units textually include support
# `.c` files (for example coverage.c includes gcov-io.c), so stage all C files
# while the committed object list still controls what is compiled separately.
# Keep source-relative paths so quoted and relative includes still work.
(
  cd "$REF/gcc-4.7.4/gcc"
  find . -path './testsuite' -prune -o -type f \
    \( -name '*.c' -o -name '*.h' -o -name '*.def' -o -name '*.md' \
       -o -name '*.opt' -o -name '*.inc' \) -print0 |
    xargs -0 -r cp --parents -t "$UP/gcc"
)

# Copy exactly the C translation units represented by the committed object list.
while IFS= read -r obj; do
  stem="${obj%.o}"
  src=""
  for cand in \
    "$OS/contrib/gcc/gen/$stem.c" \
    "$OS/contrib/gcc/shims/$stem.c" \
    "$REF/gcc-4.7.4/gcc/$stem.c" \
    "$REF/gcc-4.7.4/gcc/config/i386/$stem.c" \
    "$REF/gcc-4.7.4/gcc/config/$stem.c"; do
    if [[ -f "$cand" ]]; then src="$cand"; break; fi
  done
  if [[ -z "$src" ]]; then
    echo "GCC_SELFHOST_MISSING_SOURCE $obj" >&2
    exit 1
  fi
  case "$src" in
    "$OS/contrib/gcc/gen/"*) rel="${src#"$OS/contrib/gcc/gen/"}"; out="$DST/gccsrc/gen/$rel" ;;
    "$OS/contrib/gcc/shims/"*) rel="${src#"$OS/contrib/gcc/shims/"}"; out="$DST/gccsrc/shims/$rel" ;;
    "$REF/gcc-4.7.4/gcc/"*) rel="${src#"$REF/gcc-4.7.4/gcc/"}"; out="$UP/gcc/$rel" ;;
    *) echo "GCC_SELFHOST_BAD_SOURCE $src" >&2; exit 1 ;;
  esac
  mkdir -p "$(dirname "$out")"
  cp "$src" "$out"
  # Normalize the source path to its committed object-list spelling. This
  # lets the same GNU Make pattern rule build flat, c-family/, common/, and
  # generated/shim objects without an in-OS shell source-search loop.
  unit="$DST/gccsrc/units/$stem.c"
  mkdir -p "$(dirname "$unit")"
  cp "$src" "$unit"
done < "$OS/contrib/gcc/cc1-objs.txt"

# GCC-bundled support libraries compiled as part of the in-OS compiler stage.
for tree in libcpp libiberty libdecnumber zlib include libgcc; do
  mkdir -p "$UP/$tree"
  (
    cd "$REF/gcc-4.7.4/$tree"
    find . -type f \( -name '*.c' -o -name '*.h' -o -name '*.inc' \
      -o -name '*.def' \) -print0 |
      xargs -0 -r cp --parents -t "$UP/$tree"
  )
done

cp -a "$OS/contrib/gcc/gen/." "$DST/gccsrc/gen/"
cp -a "$OS/contrib/gcc/shims/." "$DST/gccsrc/shims/"
cp "$OS/contrib/gcc/cc1-objs.txt" "$DST/gccsrc/cc1-objs.txt"
cp "$OS/contrib/gcc/Selfhost.mk" "$DST/gccsrc/Selfhost.mk"
{
  printf '%s\n' '# Generated from the committed GCC closure object list.'
  while IFS= read -r obj; do
    [[ -n "$obj" ]] && printf 'CC1_NAMES += %s\n' "$obj"
  done < "$OS/contrib/gcc/cc1-objs.txt"
} > "$DST/gccsrc/cc1-objs.mk"
cp "$OS/contrib/gcc/config.h" "$OS/contrib/gcc/localedir.h" \
  "$DST/gccsrc/conf/gcc/"
cp -a "$OS/contrib/gcc/decnuminc/." "$DST/gccsrc/conf/decnumber/"

# SDK sources/headers are rebuilt in-OS for the new cc1 and driver runtime.
cp -a "$OS/sdk/include" "$DST/gccsrc/sdk/"
cp "$OS/sdk/crt1.c" "$OS/sdk/tccsdk.c" "$OS/sdk/libtcc1.c" \
  "$OS/sdk/posix.c" "$OS/sdk/setjmp.c" "$DST/gccsrc/sdk/"
find "$OS/sdk" -maxdepth 1 -type f -name '*.h' \
  -exec cp {} "$DST/gccsrc/sdk/" \;
cp "$OS/contrib/gccdriver/gccdriver.c" "$DST/gccsrc/gccdriver.c"

# Configured public headers and generated tables used by GCC's math runtime.
find "$OS/contrib/gmp" -maxdepth 1 -type f \( -name '*.h' -o -name '*.c' \) \
  -exec cp {} "$DST/gccsrc/conf/gmp/" \;
find "$OS/contrib/mpfr" -maxdepth 1 -type f -name '*.h' \
  -exec cp {} "$DST/gccsrc/conf/mpfr/" \;
find "$OS/contrib/mpc" -maxdepth 1 -type f -name '*.h' \
  -exec cp {} "$DST/gccsrc/conf/mpc/" \;
find "$REF/mpfr-3.0.1" -maxdepth 1 -type f -name '*.h' \
  -exec cp {} "$UP/mpfr/" \;
find "$REF/mpc-1.0.1/src" -maxdepth 1 -type f -name '*.h' \
  -exec cp {} "$UP/mpc/src/" \;

# GMP/MPFR/MPC are external prerequisite libraries, not GCC source. They are
# the stage-1 bootstrap seed; all GCC-owned objects are rebuilt inside ICS-OS.
cp "$OS/contrib/gmp/libgmp.a" "$OS/contrib/mpfr/libmpfr.a" \
  "$OS/contrib/mpc/libmpc.a" "$DST/seed/"

printf 'staged GCC closure source: %s files, %s KiB\n' \
  "$(find "$DST" -type f | wc -l)" "$(du -sk "$DST" | awk '{print $1}')"
