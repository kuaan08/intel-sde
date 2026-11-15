#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$ROOT/build"
CXX="${CXX:-x86_64-linux-gnu-g++}"
QEMU="${QEMU:-qemu-x86_64}"
QEMU_LD_PREFIX="${QEMU_LD_PREFIX:-/usr/x86_64-linux-gnu}"
OBJDUMP="${OBJDUMP:-x86_64-linux-gnu-objdump}"
mkdir -p "$BUILD"
"$CXX" -std=c++17 -O2 -Wall -Wextra -Wpedantic \
  -march=x86-64 -msse4.2 -mno-avx -mno-avx2 -mno-fma -mno-avx512f \
  -fno-tree-vectorize -fno-lto "$ROOT/src/avx_runtime.cpp" -o "$BUILD/avx_runtime"
"$QEMU" -L "$QEMU_LD_PREFIX" "$BUILD/avx_runtime" --self-test
"$QEMU" -L "$QEMU_LD_PREFIX" "$BUILD/avx_runtime" --decode "c4 41 34 58 e6"
"$QEMU" -L "$QEMU_LD_PREFIX" "$BUILD/avx_runtime" --run-file "$ROOT/tests/sample-program.txt"
if "$OBJDUMP" -d "$BUILD/avx_runtime" | grep -Eiq '\b(v[a-z][a-z0-9]*)\b'; then
  echo 'ERROR: V-prefixed AVX mnemonic found in runtime binary' >&2
  exit 1
fi
echo 'PASS: independent interpreter built without AVX mnemonics.'
