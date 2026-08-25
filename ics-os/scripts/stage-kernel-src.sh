#!/bin/bash
# Pack kernel C sources + GAS objects for in-OS kbuild.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DST="${1:-/tmp/icsos-kbuild}"
K="$ROOT/kernel"

rm -rf "$DST"
mkdir -p "$DST/src" "$DST/kasm"

# C/H/S tree without build products.
( cd "$K" && tar cf - \
    --exclude='*.o' --exclude='Kernel*' --exclude='vmdex' \
    --exclude='mapfile.txt' --exclude='*.sym' \
    . ) | ( cd "$DST/src" && tar xf - )

# Preassembled GAS (tcc cannot assemble our .S files).
gcc -c -m64 -o "$DST/kasm/mbhdr.o" "$K/startup/mbhdr.S"
gcc -c -m64 -o "$DST/kasm/startup.o" "$K/startup/startup.S"
gcc -c -m64 -o "$DST/kasm/asmlib.o" "$K/startup/asmlib.S"
gcc -c -m64 -o "$DST/kasm/irqwrap.o" "$K/irqwrap.S"
gcc -c -m64 -o "$DST/kasm/context.o" "$K/cpu/context.S"
gcc -c -m64 -o "$DST/kasm/ap_trampoline.o" "$K/cpu/ap_trampoline.S"
gcc -c -m64 -ffreestanding -fno-builtin -o "$DST/kasm/tccva.o" "$K/tccva.c"

( cd "$DST/src" && tar --format=ustar -cf "$DST/ksrc.tar" . )
echo "staged kernel sources under $DST (ksrc.tar $(wc -c < "$DST/ksrc.tar") bytes)"
