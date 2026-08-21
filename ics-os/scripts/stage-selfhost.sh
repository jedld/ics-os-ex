#!/bin/bash
# Stage TinyCC, POSIX headers, kernel sources and self-host tests into an
# ICS-OS image root (the directory passed as $1, default tmp).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
STAGE="${1:-$ROOT/tmp}"

mkdir -p "$STAGE/apps" "$STAGE/tcc1/include" "$STAGE/include" \
         "$STAGE/src/kernel" "$STAGE/src/tcc" "$STAGE/src/tests" "$STAGE/src/sdk"

# POSIX + TCC headers for programs compiled inside the OS.
cp -a "$ROOT/sdk/include/." "$STAGE/include/"
cp -a "$ROOT/contrib/tcc/include/." "$STAGE/tcc1/include/" 2>/dev/null || true

# SDK sources that tcc links into user programs.
cp -a "$ROOT/sdk/"*.c "$STAGE/tcc1/" 2>/dev/null || true
cp -a "$ROOT/sdk/"*.h "$STAGE/tcc1/" 2>/dev/null || true
cp -a "$ROOT/sdk/"*.S "$STAGE/tcc1/" 2>/dev/null || true
cp -a "$ROOT/sdk/"*.mk "$STAGE/tcc1/" 2>/dev/null || true

# TinyCC sources (so tcc can compile itself).
cp -a "$ROOT/contrib/tcc/"*.c "$ROOT/contrib/tcc/"*.h \
      "$ROOT/contrib/tcc/"*.def "$STAGE/src/tcc/" 2>/dev/null || true
mkdir -p "$STAGE/src/tcc/include" "$STAGE/src/tcc/lib"
cp -a "$ROOT/contrib/tcc/include/." "$STAGE/src/tcc/include/"
cp -a "$ROOT/contrib/tcc/lib/." "$STAGE/src/tcc/lib/"
cp -a "$ROOT/contrib/tcc/Makefile" "$STAGE/src/tcc/" 2>/dev/null || true

# Kernel sources needed to rebuild Kernel32.bin inside the OS.
# Keep directory layout so #include "filesystem/fat12.h" works.
( cd "$ROOT/kernel" && tar cf - \
    --exclude='*.o' --exclude='Kernel32.bin' --exclude='Kernel32.sym' \
    --exclude='vmdex' --exclude='mapfile.txt' --exclude='mnt' \
    . ) | ( cd "$STAGE/src/kernel" && tar xf - )

# Self-host smoke-test programs.
cp -a "$ROOT/contrib/selfhost/hello.c" "$STAGE/src/tests/hello.c"
cp -a "$ROOT/contrib/selfhost/min.c" "$STAGE/apps/min.c"
cp -a "$ROOT/contrib/selfhost/hello.c" "$STAGE/apps/hello.c"
cp -a "$ROOT/contrib/selfhost/tinyio.c" "$STAGE/apps/tinyio.c"
cp -a "$ROOT/contrib/selfhost/tinycrt.c" "$STAGE/apps/tinycrt.c"

# PATH/SDK for the in-OS compiler. Optionally run the self-host test on boot.
AUTO="$STAGE/autoexec.bat"
if [ -f "$AUTO" ]; then
    if ! grep -q 'SDK_HOME' "$AUTO"; then
        cat >> "$AUTO" << 'EOF'
set PATH=/icsos/apps
set SDK_HOME=/icsos/tcc1
EOF
    fi
    if [ "${ICSOS_TEST_SELFHOST:-}" = "1" ]; then
        cat >> "$AUTO" << 'EOF'
selfhost
reboot
EOF
    fi
    if [ "${ICSOS_TEST_IOBENCH:-}" = "1" ]; then
        cat >> "$AUTO" << 'EOF'
iobench
reboot
EOF
    fi
    if [ "${ICSOS_TEST_TCCBOOT:-}" = "1" ]; then
        cat >> "$AUTO" << 'EOF'
tccboot
reboot
EOF
    fi
    if [ "${ICSOS_TEST_EXEC:-}" = "1" ]; then
        cat >> "$AUTO" << 'EOF'
exectest
reboot
EOF
    fi
fi

echo "staged self-host sources under $STAGE"
