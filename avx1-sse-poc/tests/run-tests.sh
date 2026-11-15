#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$ROOT/build"
CXX="${CXX:-x86_64-linux-gnu-g++}"
OBJDUMP="${OBJDUMP:-x86_64-linux-gnu-objdump}"
QEMU="${QEMU:-qemu-x86_64}"
QEMU_CPU="${QEMU_CPU:-max,-avx,-avx2,-xsave}"
QEMU_LD_PREFIX="${QEMU_LD_PREFIX:-/usr/x86_64-linux-gnu}"
mkdir -p "$BUILD"

"$CXX" -std=c++17 -O2 -Wall -Wextra -Wpedantic \
  -march=x86-64 -msse4.2 -mno-avx -mno-avx2 -mno-fma -mno-avx512f \
  -fno-tree-vectorize -fno-lto \
  "$ROOT/src/avx1_sse_poc.cpp" -o "$BUILD/avx1_sse_poc"

# These fixture instructions are assembled and disassembled only; never execute this AVX object.
"$CXX" -c -mavx "$ROOT/tests/avx1_encoding_fixtures.S" -o "$BUILD/avx1_encoding_fixtures.o"
"$OBJDUMP" -d -M intel "$BUILD/avx1_encoding_fixtures.o" > "$BUILD/avx1_encoding_fixtures.disasm"
for bytes in 'c5 f0 58 c2' 'c4 41 34 58 e6' 'c5 f8 10 df' 'c5 f8 77'; do
  if ! grep -Fqi "$bytes" "$BUILD/avx1_encoding_fixtures.disasm"; then
    echo "ERROR: GNU assembler fixture bytes not found: $bytes" >&2
    exit 1
  fi
done

"$QEMU" -cpu "$QEMU_CPU" -L "$QEMU_LD_PREFIX" \
  "$BUILD/avx1_sse_poc" --dump "$BUILD/generated_addps256_sse.bin"

"$OBJDUMP" -D -b binary -m i386:x86-64 \
  "$BUILD/generated_addps256_sse.bin" > "$BUILD/generated_addps256_sse.disasm"
"$OBJDUMP" -d "$BUILD/avx1_sse_poc" > "$BUILD/harness.disasm"

if grep -E '\b(v[a-z][a-z0-9]*)\b' "$BUILD/generated_addps256_sse.disasm" "$BUILD/harness.disasm"; then
  echo "ERROR: VEX/AVX mnemonic found in generated code or harness" >&2
  exit 1
fi
if ! grep -Eq '\b(movups|addps|subps|mulps|divps|xorps|andps|orps)\b' "$BUILD/generated_addps256_sse.disasm"; then
  echo "ERROR: expected legacy SSE instructions were not found in generated dump" >&2
  exit 1
fi

echo "PASS: generated code and harness disassembly contain no V-prefixed AVX mnemonics."
echo "Disassembly: $BUILD/generated_addps256_sse.disasm"
