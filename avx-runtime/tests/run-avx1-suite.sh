#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${BUILD:-$ROOT/build}"
QEMU="${QEMU:-qemu-x86_64}"
QEMU_LD_PREFIX="${QEMU_LD_PREFIX:-/usr/x86_64-linux-gnu}"
BIN="$BUILD/avx_runtime"
[ -x "$BIN" ] || { echo "Build the runtime first: tests/run-tests.sh" >&2; exit 2; }
for program in "$ROOT/tests/programs"/*.txt; do
  output="$($QEMU -L "$QEMU_LD_PREFIX" "$BIN" --run-file "$program")"
  grep -q '^PASS: executed ' <<<"$output"
  echo "PASS: $program"
done
