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
  -fno-tree-vectorize -fno-lto -I"$ROOT/include" \
  "$ROOT/src/avx_runtime.cpp" "$ROOT/src/memory_address.cpp" "$ROOT/src/block_cache.cpp" "$ROOT/src/block_builder.cpp" "$ROOT/src/executable_memory.cpp" "$ROOT/src/feature_policy.cpp" "$ROOT/src/windows_process_host.cpp" \
  -o "$BUILD/avx_runtime"
"$QEMU" -L "$QEMU_LD_PREFIX" "$BUILD/avx_runtime" --self-test
benchmark_output="$($QEMU -L "$QEMU_LD_PREFIX" "$BUILD/avx_runtime" --benchmark 10000)"
grep -q '^benchmark.operations=' <<<"$benchmark_output"
echo "$benchmark_output"
"$QEMU" -L "$QEMU_LD_PREFIX" "$BUILD/avx_runtime" --decode "c4 41 34 58 e6"
"$QEMU" -L "$QEMU_LD_PREFIX" "$BUILD/avx_runtime" --run-file "$ROOT/tests/sample-program.txt"
printf '# CRLF portability test\r\nc5 f0 58 c2\r\n' > "$BUILD/crlf-program.txt"
"$QEMU" -L "$QEMU_LD_PREFIX" "$BUILD/avx_runtime" --run-file "$BUILD/crlf-program.txt"
if "$QEMU" -L "$QEMU_LD_PREFIX" "$BUILD/avx_runtime" --decode "c4 01 34 58 e6"; then
  echo 'ERROR: reserved VEX.X encoding was accepted' >&2
  exit 1
fi
if "$QEMU" -L "$QEMU_LD_PREFIX" "$BUILD/avx_runtime" --decode "c5 f0 58 c-"; then
  echo 'ERROR: invalid hexadecimal input was accepted' >&2
  exit 1
fi
if "$OBJDUMP" -d "$BUILD/avx_runtime" | grep -Eiq '\b(v[a-z][a-z0-9]*)\b'; then
  echo 'ERROR: V-prefixed AVX mnemonic found in runtime binary' >&2
  exit 1
fi
echo 'PASS: independent interpreter built without AVX mnemonics.'
