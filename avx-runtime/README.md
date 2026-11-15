# SDE AVX Lab — Independent AVX1 Runtime

> Original project code. No Intel SDE implementation code is used.

This directory contains the correctness-first core of the independent SDE AVX Lab emulator. It decodes a controlled subset of VEX-encoded AVX1 instructions and executes them with a scalar host backend that does not require AVX on the host CPU.

## Supported prototype scope

- `VADDPS`, `VSUBPS`, `VMULPS`, and `VDIVPS`;
- `VXORPS`, `VANDPS`, and `VORPS`;
- register-register `VMOVUPS` and `VMOVAPS`, including store encodings;
- synthetic guest-memory loads and stores for the supported AVX1 forms;
- VEX.128 and VEX.256 widths;
- ModRM/SIB, displacement, and RIP-relative x64 address decoding;
- 16 logical YMM registers and a deterministic guest GPR/memory state for tests;
- scalar C++ execution without AVX host instructions;
- sequential instruction programs stored as text files;
- deterministic state digest output;
- shared IR data types, coverage policy, and translated-block cache invalidation prototype.

The synthetic memory machine is a core test harness, not yet a process-level Windows DBT. It must not be advertised as finished game compatibility until guest process memory, executable-page discovery, basic-block translation, code-cache invalidation, thread/exception integration, and CPUID/XGETBV interception are complete.

## Linux build and test

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
./build/avx_runtime --self-test
./build/avx_runtime --decode "c5 f0 58 00"
./build/avx_runtime --run-file tests/programs/memory-avx1.txt
```

On the ARM64 development sandbox, `tests/run-tests.sh` uses an x86-64 cross-compiler and QEMU user-mode. It disables AVX, AVX2, FMA, and AVX-512 code generation and audits the generated binary for V-prefixed instructions.

## Windows build

Requirements: Windows 10/11 x64, CMake 3.16 or newer, and a Visual Studio x64 C++ generator.

```bat
Build-Windows.bat
build-windows\Release\avx_runtime.exe --self-test
build-windows\Release\avx_runtime.exe --run-file tests\programs\memory-avx1.txt
```

Decode a VEX instruction without executing it:

```bat
build-windows\Release\avx_runtime.exe --decode "c5 f0 58 00"
```

## Program file format

One instruction per line, written as hexadecimal bytes. Spaces, tabs, colons, and Windows CRLF endings are accepted. Lines beginning with `#` are comments.

For the deterministic memory test harness, `RAX` points to guest address `0x100` and `RCX` points to guest address `0x200`.

```text
# VADDPS xmm0,xmm1,[rax]
c5 f0 58 00
# VMOVUPS [rcx],xmm0
c5 f8 11 01
```

The runtime seeds guest vectors, GPRs, and memory, executes the sequence, and prints the final digest and selected lanes. This format provides a reproducible decoder/interpreter test before process-level DBT is introduced.

## Next implementation layers

The detailed phase plan, dependencies, and acceptance gates are maintained in [`docs/NEXT-MILESTONES.md`](../docs/NEXT-MILESTONES.md). The immediate sequence is:

1. complete IR read/write, memory, alignment, and exception metadata;
2. add decoder negative tests and broaden the AVX1 semantic subset;
3. translate complete straight-line blocks and identify control-flow boundaries;
4. lower the shared IR to SSE4.2/scalar host sequences and compare every block with the interpreter;
5. connect the block cache to executable process pages and self-modifying-code invalidation;
6. implement safe Windows guest-memory access, per-thread state, loader/JIT discovery, and controlled SEH/VEH chaining;
7. add CPUID/XGETBV virtualization only after process execution coverage is sufficient;
8. build the AVX1 Windows application suite before enabling AVX2 milestones.
