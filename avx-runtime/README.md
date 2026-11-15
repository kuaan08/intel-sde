# SDE AVX Lab — Independent AVX1 Runtime

> Original project code. No Intel SDE implementation code is used.

This directory contains the first executable component of the independent SDE AVX Lab emulator. It is a correctness-first interpreter for a controlled subset of VEX-encoded AVX1 instructions.

## Supported prototype scope

- `VADDPS`, `VSUBPS`, `VMULPS`, and `VDIVPS`;
- `VXORPS`, `VANDPS`, and `VORPS`;
- register-register `VMOVUPS` and `VMOVAPS`;
- `VZEROUPPER`;
- VEX.128 and VEX.256 widths;
- 16 logical YMM registers;
- scalar C++ execution without AVX host instructions;
- sequential instruction programs stored as text files;
- deterministic state digest output.

This is an independent instruction emulator, not yet a process-level Windows DBT. It must not be advertised as finished game compatibility until memory operands, basic-block translation, process interception, JIT invalidation, and exception handling are implemented.

## Linux build and test

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/avx_runtime --self-test
./build/avx_runtime --decode "c5 f0 58 c2"
./build/avx_runtime --run-file tests/sample-program.txt
```

On the ARM64 development sandbox, `tests/run-tests.sh` uses an x86-64 cross-compiler and QEMU user-mode. It disables AVX, AVX2, FMA, and AVX-512 code generation and audits the generated binary for V-prefixed instructions.

## Windows build

Requirements: Windows 10/11 x64, CMake 3.16 or newer, and a Visual Studio x64 C++ generator.

```bat
Build-Windows.bat
build-windows\Release\avx_runtime.exe --self-test
build-windows\Release\avx_runtime.exe --run-file tests\sample-program.txt
```

Decode a VEX instruction without executing it:

```bat
build-windows\Release\avx_runtime.exe --decode "c4 41 34 58 e6"
```

## Program file format

One instruction per line, written as hexadecimal bytes. Spaces, tabs, and colons are accepted. Lines beginning with `#` are comments.

```text
# VADDPS xmm0,xmm1,xmm2
c5 f0 58 c2
# VXORPS ymm3,ymm3,ymm3
c5 e4 57 db
# VZEROUPPER
c5 f8 77
```

The runtime seeds a deterministic guest state, executes the sequence, and prints the final digest and selected lanes. This format provides a reproducible decoder/interpreter test before process-level DBT is introduced.

## Next implementation layers

1. memory operands, ModRM/SIB, and x64 address calculation;
2. a shared intermediate representation for interpretation and compilation;
3. complete basic-block translation with control-flow boundaries;
4. translated-code cache and self-modifying-code invalidation;
5. independent Windows process interception;
6. CPUID/XGETBV virtualization gated by actual coverage;
7. AVX1 application tests followed by AVX2 milestones.
