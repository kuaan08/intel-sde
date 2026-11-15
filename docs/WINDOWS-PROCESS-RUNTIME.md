# Windows Process Runtime

The independent backend now contains the first process-hosting layer for Windows x64.

## Current implementation

- starts a target process suspended with `CreateProcessW`;
- injects `avx_winshim.dll` through a remote `LoadLibraryW` thread;
- resumes and waits for the target process;
- installs a vectored exception handler inside the target;
- catches `EXCEPTION_ILLEGAL_INSTRUCTION`;
- emulates a small VEX.128 AVX1 subset through the Windows `CONTEXT` XMM registers;
- resolves basic ModRM/SIB, base/index, displacement, and RIP-relative memory operands with guarded reads/writes;
- supports the initial arithmetic, logical, move, and `VZEROUPPER` forms implemented by the shim.

The launcher uses this path when the own backend is selected and both a target executable and `avx_winshim.dll` are available beside `avx_runtime.exe`. Without the shim, it falls back to the deterministic self-test.

## Deliberate limits

This is an early process-runtime milestone, not full application compatibility. It does not yet translate AVX.256/YMM upper state, AVX2, CPUID/XGETBV results, indirect control flow, generated code invalidation, loader notifications, or all thread contexts. Unsupported illegal instructions are passed to the next Windows exception handler and the target may terminate normally.

The shim is intentionally conservative. It must never claim that a target is supported merely because it starts. Each new instruction family needs a Windows fixture and a differential result before it can be enabled in the process path.

## Next Windows layers

1. capture and register every target thread, including loader-created threads;
2. add safe process-memory reads/writes for ModRM/SIB forms;
3. discover executable pages with `VirtualQuery` and track `VirtualProtect`/JIT writes;
4. invalidate translated blocks on overlapping writes;
5. add complete CPUID/XGETBV policy virtualization through a controlled interception strategy;
6. add SEH/VEH diagnostics, crash reports, and per-thread vector state;
7. extend from VEX.128 to VEX.256 and then AVX2 only after differential coverage.
