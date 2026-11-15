# Xenia AVX Interceptor Test — 2026-11-15

## Scope

This report validates the current experimental Windows process host and its opt-in `CPUID`/`XGETBV` feature-query path against the Xenia Canary release previously downloaded for analysis. It does not claim complete Xenia compatibility or complete process-level DBT.

## Test environment

- Xenia Canary release: `9eedf60`, Windows x64 package.
- Host environment: Linux ARM64 sandbox.
- Windows execution layer: Wine 64-bit.
- CPU model exposed to the guest process: QEMU user-mode `Conroe`.
- AVX, AVX2, and FMA host code generation disabled when building the SDE AVX Lab launcher and shim.
- SDE AVX Lab launcher: current `master` source with the experimental shim.
- Timeout: 45 seconds for the Xenia process tests.

This is a cross-emulated validation environment. It is not native Windows 10, native Windows 11, or the reference Celeron N5100.

## Test matrix

| Case | Command path | Result |
|---|---|---|
| Direct Xenia | Xenia under Wine/QEMU Conroe without the SDE AVX Lab shim | Process initialized far enough to load its configuration and remained active until the timeout. |
| Launcher, default mode | SDE AVX Lab process host without `SDE_AVX_LAB_EXPERIMENTAL_CPUID=1` | Target process started, but no AVX translation or complete DBT was demonstrated. |
| CPUID fixture | Windows fixture under the launcher with `SDE_AVX_LAB_EXPERIMENTAL_CPUID=1` | **PASS**: virtual AVX/OSXSAVE and `XCR0=0x6`; exit status 0. |
| Post-startup JIT fixture | Fixture allocates writable memory, publishes it as executable, emits VEX.128 `VADDPS`, and executes through the injected shim | **PASS**: `11.0 22.0 33.0 44.0`; exit status 0. |
| Multithread JIT fixture | Four Windows worker threads execute the same post-startup VEX.128 JIT page concurrently | **PASS**: `3.0 14.0 108.0 -2.0`; exit status 0. |
| Exception-chain fixture | Shim declines an intentionally unsupported VEX form; a later target VEH skips it and resumes at a safe instruction | **PASS**: `result=0 handled=1`; exit status 0. |
| Xenia, CPUID opt-in | Xenia under the launcher with `SDE_AVX_LAB_EXPERIMENTAL_CPUID=1` | The previous breakpoint failure was fixed. Xenia remained active through the 45-second timeout without the AVX startup rejection or an unhandled `INT3` from the feature-query path. |

The CPUID fixture printed the following result:

```text
CPUID fixture: max_leaf=11538209 avx=1 osxsave=1 xcr0=0x6
```

Wine emitted unrelated environment warnings, including missing `syswow64\\rundll32.exe`, RPC/COM setup warnings, and HID stubs. These warnings are limitations of the test environment and are not evidence of SDE AVX Lab translation correctness.

## What this proves

The opt-in main-image patch path can replace the first byte of `CPUID` and `XGETBV` with `INT3`, receive both RIP conventions used by the Wine/QEMU execution path, emulate AVX and OSXSAVE feature bits, return a virtual `XCR0=0x6`, and resume execution at the correct following instruction. The corrected path also allowed the tested Xenia process to pass beyond the feature-query breakpoint stage.

## What this does not prove

The current implementation is still not a complete process-level DBT. The patcher covers only the main executable image, not all DLLs or JIT-generated executable pages. The VEH shim supports only a narrow VEX.128 subset and does not lift or translate arbitrary Xenia x64 JIT blocks. It does not yet provide complete memory helpers, control-flow block execution, YMM state preservation for all paths, MXCSR/exception fidelity, loader/JIT lifecycle handling, thread synchronization, or SEH/VEH chaining.

The 45-second Xenia timeout is therefore a liveness observation, not a compatibility result. No game was loaded, no guest title was executed, and no correctness or FPS claim is made.

## Next engineering gate

The controlled JIT fixture now passes through the injected VEH path. It proves that a supported VEX.128 instruction in a page created after process startup can be intercepted and executed. It does not yet prove that the process host discovers the page, creates an owned translated block, caches it, invalidates it after a write, or translates arbitrary Xenia JIT blocks. Those are the next gates before broader Xenia JIT coverage and differential testing.

The process tracker was also hardened to retain live executable-region generations, notice region-size changes, and remove regions no longer returned by `VirtualQuery`. This core behavior is covered by the no-AVX build, but remote-process write hooks and target-side cache ownership still require a Windows integration gate.
