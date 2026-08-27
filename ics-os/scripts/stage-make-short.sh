#!/bin/bash
# Stage a slim GNU Make 3.82 tree for in-OS TinyCC: preprocessed .c, SDK
# objects, and a tiny Makefile. Packed as one ustar (makesrc.tar) so the
# CD is a single map. Host tcc -E so in-OS tcc never opens SDK headers.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DST="${1:-/tmp/icsos-make-short}"
SDK="$ROOT/sdk"

chmod +x "$ROOT/scripts/stage-make.sh"
"$ROOT/scripts/stage-make.sh" "$DST/src"

rm -rf "$DST/pre" "$DST/sdkobj"
mkdir -p "$DST/pre" "$DST/sdkobj"

MAKE_C="ar arscan commands default dir expand file function getopt getopt1
        implicit job main misc read remake rule signame strcache variable
        version vpath hash remote-stub"
INC="-nostdinc -DHAVE_CONFIG_H -I$DST/src -I$DST/src/glob -I$SDK/include"

echo "host: preprocessing GNU make with tcc -E"
pre_one() {
  local src="$1" dest="$2"
  tcc -E $INC "$src" | python3 -c '
import sys
s = sys.stdin.read()
lines = []
for ln in s.splitlines(True):
    if ln.startswith("#"):
        continue
    lines.append(ln)
s = "".join(lines)
s = s.replace("__builtin_alloca", "icsos_alloca")
s = s.replace("extern char *icsos_alloca ();",
              "extern void *icsos_alloca(unsigned long);")
sys.stdout.write(s)
' > "$dest"
}
for s in $MAKE_C; do
  out="$s"
  [ "$s" = "remote-stub" ] && out="remstub"
  pre_one "$DST/src/${s}.c" "$DST/pre/${out}.c"
done
pre_one "$DST/src/glob/glob.c" "$DST/pre/glob.c"
pre_one "$DST/src/glob/fnmatch.c" "$DST/pre/fnmatch.c"

SDKOBJ_CFLAGS="-m64 -std=gnu89 -w -nostdlib -fno-builtin -static -ffreestanding \
  -fno-pie -fno-pic -fno-stack-protector -fno-asynchronous-unwind-tables \
  -fno-unwind-tables -fno-stack-clash-protection \
  -fno-strict-aliasing -mcmodel=large \
  -mno-red-zone -nostdinc -I$SDK/include -I$ROOT/contrib/tcc \
  -DTCC_TARGET_X86_64 -DONE_SOURCE=0 -DCONFIG_TCC_STATIC"
for f in tccsdk posix libtcc1 crt1 setjmp; do
  echo "host: building SDK runtime object $f.o"
  gcc -c $SDKOBJ_CFLAGS -o "$DST/sdkobj/$f.o" "$SDK/$f.c"
done

cp "$ROOT/contrib/gnumake/t.mk" "$DST/t.mk"

# Drop the full source tree from the tar; in-OS tcc only needs pre/ + sdkobj.
rm -rf "$DST/src"

echo "staged makeboot tree under $DST (pre=$(ls -1 "$DST/pre" | wc -l))"
