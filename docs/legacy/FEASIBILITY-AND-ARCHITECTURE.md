# Feasibility and Architecture Record

> Historical project record. Current implementation status is maintained in the root [README](../../README.md) and [Next Engineering Milestones](../NEXT-MILESTONES.md).

## Verified project baseline

The original handoff was audited before implementation began. It contained launcher source, batch and PowerShell scripts, status notes, research material, and execution logs without evidence of a complete AVX translator. The initial concrete result was that Intel SDE 9.58.0 opened Xenia Canary with the `-snb` profile. No game execution, gameplay stability, or playable frame rate was established by that observation.

The reference platform is Windows 10 x64 on an Intel Celeron N5100. Intel lists SSE4.2 for this processor and does not list AVX. Physical reference hardware was not available during the initial audit. Intel SDE 10.13.1 reported `Create pind process failed with code 5` in the reported Windows 10 experiment; the cause was not proven.

The original handoff was a launcher and experiment record, not a DBT implementation. It did not contain a production decoder, translator, SSE backend, JIT cache, or automated equivalence suite.

## Architecture decision

No open-source product was found that transparently executes every Windows x64 AVX/AVX2 application on hardware without AVX. The technically viable direction is an independent process runtime with a dynamic translation layer, semantic IR, explicit vector state, and an SSE/scalar backend.

DynamoRIO remains a candidate DBI and code-cache foundation for a Windows prototype. Its instrumentation and block transformation APIs are useful, but it does not provide AVX-to-SSE semantic equivalence. Intel XED is suitable for decoding and encoding tools, not process management. QEMU TCG is a valuable reference for IR, translation blocks, cache invalidation, and exception handling, but QEMU user-mode is not a native Windows PE/WinAPI compatibility layer.

| Component | Intended role | Boundary |
|---|---|---|
| DynamoRIO | DBI, block transformation, code cache, process and child tracking | Must prove that AVX blocks are intercepted before execution on a host without AVX. |
| Intel XED | x86/VEX decoding and encoding support | Apache-2.0 must be reviewed together with bundled build dependencies. |
| QEMU TCG | Reference for IR, translation blocks, cache, and invalidation | QEMU is GPL-2.0 overall; reuse requires file-level license review. |
| Intel Pin/SDE | External behavioral comparison | Proprietary runtime; do not modify or copy implementation code. |
| QBDI and Frida Gum | Alternative instrumentation experiments | Neither provides a complete AVX-to-SSE backend. |
| LLVM ORC | Possible future JIT backend | Useful only after x86 lifting and semantic IR exist. |

## Required runtime layers

1. **Process DBI:** capture executable blocks before unsupported instructions execute, maintain a code cache, and observe code changes.
2. **Decoder and classification:** identify VEX prefixes, vector width, operands, addressing, register effects, memory effects, and exceptions.
3. **Semantic IR:** represent operations, operands, zeroing behavior, memory width, MXCSR effects, and exception metadata.
4. **SSE/scalar backend:** lower only to instructions supported by the host and preserve 128-bit and 256-bit semantics.
5. **Per-thread vector state:** model all logical YMM registers, calls, callbacks, preemption, and exception entry/exit.
6. **Capability model:** keep CPUID, XGETBV, OSXSAVE, and XSAVE behavior consistent with actual translation coverage.
7. **Dynamic code lifecycle:** handle DLLs, JIT pages, child processes, threads, calls, returns, invalidation, and structured exceptions.

## Proof-of-concept result

The project now contains an independent AVX1 interpreter and an AVX1-to-legacy-SSE proof of concept. The controlled subset includes packed-single arithmetic and logical operations, moves, `VZEROUPPER`, VEX.128/VEX.256 state, and selected memory forms in the core harness. The Windows shim also has a narrow experimental VEX.128 path and scalar-single coverage.

The core was compiled for x86-64 with AVX disabled and executed through QEMU user-mode on the ARM64 development environment. The controlled tests compared state against a scalar reference and audited generated code for unsupported V-prefixed host instructions. This validates only the covered subset under emulation; it is not native Celeron N5100 or Windows reference-hardware validation.

## Open acceptance gates

- prove process-level DBI interception on Windows x64;
- replace the narrow parser with a production-grade decoder or a reviewed decoder dependency;
- add memory, MXCSR, exception, alignment, and page-fault equivalence tests;
- integrate per-thread state, loader/JIT discovery, child processes, and SEH/VEH chaining;
- validate CPUID/XGETBV only after execution coverage is sufficient;
- test real AVX1 applications before evaluating Xenia or games;
- measure translation, cache, helper, and execution overhead separately.

The compatibility objective is technically investigable, but it remains an engineering goal rather than a current capability claim.
