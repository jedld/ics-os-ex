#!/usr/bin/env bash
# Run ICS-OS QEMU integration tests (boot + SMP + ELF64 exec).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
echo "ICS-OS integration tests (cwd=$ROOT)"
make test-integration
