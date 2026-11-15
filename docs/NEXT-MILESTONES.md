# Next Engineering Milestones

## Memory and translation core

1. Extend the VEX decoder to produce `IRInstruction` for ModRM/SIB memory operands.
2. Add safe guest-memory read/write helpers with x64 overflow and page-boundary checks.
3. Translate one straight-line block into IR, preserving original address and instruction length.
4. Add a code-cache key containing module identity, guest address, source bytes, and generation.
5. Invalidate overlapping blocks after executable-page writes and flush platform instruction caches.

## Windows process runtime

The next Windows-only layer must discover executable modules, create per-thread translation state, observe loader-created threads, and route illegal-instruction faults through a controlled exception path. It must not claim arbitrary-game support until thread creation, SEH/VEH, loader events, JIT pages, and code invalidation are covered by tests.

## CPUID/XGETBV gating

The current `feature_policy` module intentionally derives AVX visibility from coverage. The future Windows interceptor will expose AVX only when the corresponding decoder, memory semantics, state handling, and exception tests are enabled. AVX2 remains hidden until integer/vector AVX2 coverage is complete.

## AVX1 application suite

The checked-in text programs are deterministic decoder regressions. The next application suite will contain small Windows x64 executables compiled with AVX intrinsics, a scalar reference, and expected output hashes. It will test initialization, threads, memory operands, `vzeroupper`, and CPUID refusal on a host without AVX.

## AVX2 milestones

- integer `VPADDD`, `VPSUBD`, `VPAND`, `VPOR`, `VPXOR` register forms;
- 256-bit integer lane semantics;
- gather, permute, broadcast, and variable-shift forms;
- memory operands and alignment behavior;
- AVX2 CPUID gating only after differential tests;
- application probes before any game compatibility claim.

## Performance and packaging

Measure decode time, IR translation time, cache hit rate, interpreter fallback count, and translated instructions per second. Package only tested binaries, manifests, licenses, and the English documentation. The GitHub Actions workflow produces Windows build artifacts for every validated revision.
