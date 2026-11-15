# Legal Boundary and Independent Optimization Plan

## Purpose

SDE AVX Lab is an independent dynamic binary translation project. It may study public documentation, public behavior, compatible open-source projects, and independently authored test results. It must not become a modified or copied implementation of Intel SDE.

## Intel SDE package boundary

The `windows-9.58/` directory contains an official Intel SDE package. Its included license terms govern use and redistribution and prohibit reverse engineering, decompilation, disassembly, or other restricted activities. Accordingly, this project does not reverse-engineer, patch, extract pseudocode from, or copy implementation code from `sde.exe`, `xed.exe`, `pin.exe`, or the package DLLs.

The package may remain available for authorized external comparison, documented behavior observation, version and profile checks, and controlled compatibility experiments. An application starting under Intel SDE is not evidence that the independent SDE AVX Lab runtime works.

## Permitted independent inputs

The project may use, subject to the applicable license and attribution requirements:

- public ISA documentation and vendor programming manuals;
- public README files, command-line documentation, and release notes;
- externally observable results from authorized tests, without copying implementation;
- open-source decoders or runtime components with compatible licenses after file-level review;
- independently authored scalar, SSE, and hardware-reference tests;
- original code in `avx1-sse-poc/` and `avx-runtime/`.

License compatibility must be reviewed before copying code, linking a dependency, or redistributing an artifact. See [THIRD-PARTY-NOTICES.md](../THIRD-PARTY-NOTICES.md).

## Independent optimization plan

### 1. Measure before optimizing

Use external tools only as comparison and telemetry sources. Record the target profile, duration, instruction mix, thread count, failures, and functional state. Telemetry is not implementation code.

```bat
windows-9.58\sde.exe -version
windows-9.58\sde.exe -snb -mix -mix_format json -- "C:\path\program.exe"
```

The independent runtime must maintain separate measurements for decoding, translation, cache lookup, helper calls, and guest execution.

### 2. Complete the decoder contract

The production decoder must cover, test, and reject explicitly:

- two- and three-byte VEX prefixes;
- `VEX.L`, `vvvv`, `R`, `X`, and `B` fields;
- opcode, ModRM, SIB, displacement, and RIP-relative addressing;
- register, memory, and broadcast forms that are actually supported;
- unsupported AVX2 and AVX-512 forms;
- page-boundary and malformed-byte inputs.

The decoder must produce structured IR. Opcode strings and heuristics are not sufficient.

### 3. Lower AVX1 to SSE4.2 or scalar code

The initial correctness target is:

- `VADDPS`, `VSUBPS`, `VMULPS`, `VDIVPS`;
- `VMOVUPS`, `VMOVAPS`;
- `VXORPS`, `VANDPS`, `VORPS`;
- `VZEROUPPER`.

A 256-bit operation may be split into two 128-bit operations only when lane order, memory behavior, exceptions, and architectural state are preserved. VEX.128 writes must clear the corresponding upper YMM half.

### 4. Preserve architectural state

The runtime must model XMM/YMM state explicitly and account for:

- upper-half clearing and `VZEROUPPER`;
- MXCSR, rounding, and floating-point exceptions;
- aligned and unaligned memory access;
- aliasing between memory and registers;
- helper calls and ABI transitions;
- thread state and exception entry/exit.

### 5. Build the cache only after correctness

Translated blocks should be keyed by address, source bytes, module identity, feature profile, and executable-page generation. The cache must invalidate when JIT code changes. Cold and hot paths may be specialized only after equivalence tests and measurements exist.

No optimization is accepted without bitwise comparison or a documented, formally justified tolerance for the covered instruction set.

### 6. Validation matrix

Every supported instruction and operand form should be compared against:

1. an independent scalar reference;
2. native legacy SSE where applicable;
3. the translated sequence;
4. AVX-capable hardware when available;
5. relevant register, memory, MXCSR, and exception state.

The matrix must include normal values, NaNs, infinities, signed zero, overflow, underflow, alternate rounding, alignment variation, aliasing, and page faults as each layer becomes available. Intel SDE can provide external comparison data, but it is not the sole correctness oracle.

## Completion criteria

An optimization or compatibility claim is complete only when the repository contains:

- original source code;
- reproducible tests;
- disassembly showing that the SSE backend emits no unsupported V-prefixed instructions;
- functional equivalence for the covered forms;
- separate decode, translation, cache, and execution benchmarks;
- documented host CPU, operating system, and limitations.

The goal remains an independent dynamic translator, not a copy of Intel SDE.
