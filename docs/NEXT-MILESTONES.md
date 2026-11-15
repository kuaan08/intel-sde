# SDE AVX Lab — Engineering Roadmap

This roadmap is the implementation contract for the independent Windows x64 backend. It separates validated prototypes from work that is still planned. A milestone is only marked complete when its acceptance gate passes in CI and the result is reproducible on a no-AVX host or under the project’s no-AVX test runner.

## Status legend

- **Complete** — implemented and covered by the current regression suite.
- **In progress** — partial implementation exists, but the acceptance gate is not met.
- **Planned** — not implemented yet.
- **Blocked** — depends on an earlier milestone or on a Windows-only validation environment.

## Current release boundary

**Complete:** VEX decoder prototype, AVX1 packed-single interpreter subset, VEX.128/VEX.256 logical state, ModRM/SIB and RIP-relative address decoding, shared IR data types, cache-generation prototype, feature-coverage policy, Windows build, deterministic tests, CI packaging, and a small VEX.128 VEH fixture.

**In progress:** process-level execution. The current Windows shim is an experimental fault path for a narrow VEX.128 register subset. It is not yet a general DBT engine and does not enable arbitrary application compatibility.

**Not enabled:** process-level AVX advertising, AVX2 advertising, arbitrary-game support, or claims of complete Windows application compatibility.

## Phase 0 — Repository and reproducibility foundation

**Status: Complete**

- [x] Separate the independent native backend from the optional Intel SDE launcher.
- [x] Keep original-source, third-party, and Intel SDE license boundaries documented.
- [x] Build the runtime and launchers through reproducible scripts and GitHub Actions.
- [x] Publish Windows x64 artifacts with documentation and checksums.
- [x] Run Linux/no-AVX validation through cross-compilation and QEMU user mode.

**Acceptance gate:** a clean checkout builds the runtime, both launchers, fixtures, tests, and Windows package without requiring Intel SDE files.

## Phase 1 — Decoder, state, and shared IR

**Status: In progress**

- [x] Decode the currently supported VEX arithmetic, logical, move, and `VZEROUPPER` forms.
- [x] Decode VEX.128 and VEX.256 width and register fields.
- [x] Decode ModRM, SIB, displacement, and RIP-relative addressing.
- [x] Maintain 16 logical YMM registers, upper-lane clearing, GPRs, MXCSR-related state, and guest memory for deterministic tests.
- [x] Represent decoded linear instructions in shared IR types.
- [ ] Add explicit IR metadata for read/write sets, flags, memory width, alignment requirements, and exception behavior.
- [ ] Add decoder negative tests for truncated VEX prefixes, invalid `vvvv`, illegal reserved bits, unsupported opcodes, and address-size combinations.

**Acceptance gate:** every supported instruction has decoder, IR, interpreter, memory, aliasing, upper-lane, and invalid-encoding tests with deterministic diagnostics.

## Phase 2 — Correctness backend and AVX1 coverage

**Status: In progress**

- [x] Execute the initial packed-single AVX1 subset without AVX host instructions.
- [x] Validate register and synthetic-memory forms against scalar/SSE references.
- [x] Add the Windows VEX.128 guest fixture with exact VEX bytes in executable memory.
- [ ] Add remaining AVX1 packed-single move/arithmetic variants needed by real compiler output.
- [ ] Add scalar single/double forms, conversions, comparisons, blends, broadcasts, and selected shuffle/permute forms.
- [ ] Add NaN, infinity, signed-zero, rounding, MXCSR, denormal, exception-mask, and faulting-memory tests.
- [ ] Add differential execution against an AVX-capable reference machine or an independently trusted reference implementation.

**Acceptance gate:** each enabled instruction family has a scalar reference, a host-code audit, 128-bit and 256-bit tests where applicable, and no unexplained output divergence.

## Phase 3 — Basic-block translation and SSE backend

**Status: In progress**

- [ ] Decode complete straight-line blocks from a guest entry address.
- [ ] Stop blocks at direct/indirect branches, calls, returns, system instructions, page boundaries, unsupported instructions, and maximum length.
- [ ] Preserve guest address and source length for every translated instruction.
- [ ] Lower the shared IR to legacy SSE4.2 or scalar helper sequences.
- [ ] Define a calling convention for helper calls and preserve nonvolatile host registers.
- [ ] Implement translated-block exits for fall-through, direct branch, indirect branch, call, return, and unsupported-instruction fallback.
- [ ] Add block-level equivalence tests against the interpreter.

**Acceptance gate:** a block containing supported instructions executes through the SSE backend, returns to the correct guest address, and produces the same architectural state as the interpreter.

## Phase 4 — Code cache and executable-memory lifecycle

**Status: In progress**

- [x] Keep a generation-aware translated-block cache prototype.
- [x] Track executable regions and page write generations in the core model.
- [ ] Use a cache key containing process/module identity, guest address, source length/bytes, feature profile, and page generation.
- [ ] Detect executable-page changes from `VirtualProtect`, `WriteProcessMemory`, loader mappings, and JIT writes.
- [ ] Invalidate every block overlapping a modified byte range.
- [ ] Flush the host instruction cache after publishing translated code.
- [ ] Make cache publication and invalidation safe for concurrent threads.
- [ ] Add self-modifying-code and JIT regression programs.

**Acceptance gate:** a program that overwrites an already translated instruction executes the new bytes on the next dispatch without stale-block execution or data races.

## Phase 5 — Windows process runtime

**Status: In progress**

- [x] Start a Windows target suspended and inject the experimental shim.
- [x] Route a narrow `EXCEPTION_ILLEGAL_INSTRUCTION` case through a VEH handler.
- [ ] Create per-thread guest state and capture/restore XMM/YMM-compatible state at every translated entry/exit.
- [ ] Register existing threads and observe loader-created threads.
- [ ] Handle thread start, exit, TLS, APC, fiber, and suspend/resume interactions.
- [ ] Discover executable modules and executable pages with `VirtualQuery`.
- [ ] Handle loader notifications and dynamically loaded modules.
- [ ] Handle JIT-created executable pages and code invalidation.
- [ ] Add controlled SEH/VEH chaining, crash reports, unsupported-opcode reports, and reproducible minidump metadata.
- [ ] Define safe behavior for syscalls, callbacks, indirect branches, and native exception unwinding.

**Acceptance gate:** a multi-threaded Windows fixture with a dynamically generated code page, an intentional supported AVX instruction, an intentional unsupported instruction, and a handled exception completes with correct logs and no corruption of the host process.

## Phase 6 — CPUID/XGETBV and feature policy

**Status: In progress**

- [x] Maintain a coverage-gated policy object in the core.
- [ ] Intercept guest `CPUID` without changing the host’s physical feature view.
- [ ] Intercept guest `XGETBV` consistently with the advertised OSXSAVE/XCR0 state.
- [ ] Keep AVX hidden until process-level AVX execution, state handling, and fault recovery pass their gates.
- [ ] Keep AVX2 hidden until integer/vector AVX2 coverage and differential tests pass.
- [ ] Add probes for CPUID leaves, OSXSAVE, XCR0, and unsupported-feature refusal.

**Acceptance gate:** a guest probe sees only features that the active backend can execute, while a program requiring an unsupported feature fails closed with a diagnostic instead of an illegal host instruction.

## Phase 7 — AVX1 Windows application suite

**Status: Planned**

- [ ] Build small signed-off Windows x64 fixtures with compiler-generated AVX1 code.
- [ ] Cover register, stack, heap, aligned, unaligned, RIP-relative, and cross-page memory operands.
- [ ] Cover startup constructors, DLL loading, worker threads, TLS, callbacks, exceptions, and JIT-generated code.
- [ ] Record scalar reference output, expected digest, exit code, and instruction trace.
- [ ] Add a no-AVX host test matrix and a reference AVX host matrix.
- [ ] Add a process-level smoke test for each enabled instruction family.
- [ ] Add application compatibility profiles only after reproducible fixture results exist.

**Acceptance gate:** the suite runs successfully through the native backend on the reference no-AVX Windows machine and detects deliberate regressions.

## Phase 8 — AVX2 expansion

**Status: Planned**

1. Add integer register forms: `VPADDD`, `VPSUBD`, `VPAND`, `VPOR`, `VPXOR`.
2. Add 128-bit and 256-bit lane semantics with upper-half rules.
3. Add integer memory forms and alignment/page-fault behavior.
4. Add broadcasts, variable shifts, blends, permutes, and selected gathers.
5. Add signed/unsigned saturation and packing semantics where required.
6. Add AVX2 decoder, IR, interpreter, SSE/scalar lowering, and differential tests.
7. Gate AVX2 CPUID/XGETBV exposure only after process-level tests pass.
8. Run application probes before making any compatibility claim.

**Acceptance gate:** every exposed AVX2 family has decoder, interpreter, translated-backend, state, memory, exception, and no-AVX host tests.

## Phase 9 — Performance and observability

**Status: Planned**

- [x] Keep an interpreter throughput benchmark prototype.
- [ ] Measure decode time, IR construction time, translation time, dispatch time, helper-call count, cache hit/miss rate, invalidation rate, and interpreter fallback rate.
- [ ] Add reproducible benchmarks for cold blocks, hot blocks, memory-heavy blocks, and multi-threaded workloads.
- [ ] Add translated-code size and memory-use reporting.
- [ ] Add hot-block promotion and specialization only after correctness gates pass.
- [ ] Reduce state transitions and helper calls with measured register allocation improvements.
- [ ] Add performance budgets and regression thresholds to CI where stable.

**Acceptance gate:** each optimization has before/after correctness evidence, benchmark data, and no generated host instruction outside the declared host feature set.

## Phase 10 — Release and compatibility workflow

**Status: In progress**

- [x] Package the native runtime and the two separately labelled launchers.
- [x] Keep Intel SDE download guidance separate from native backend behavior.
- [ ] Add versioned configuration migration and per-application profiles.
- [ ] Add log export, diagnostic bundle creation, and clear unsupported-instruction reports.
- [ ] Add reproducible release notes describing tested CPU, OS, instruction coverage, and known limitations.
- [ ] Test the native backend against real applications only after Phases 3–7 gates pass.
- [ ] Record startup, loaded modules, frame time, crashes, and correctness observations separately for native and Intel SDE runs.

**Release gate:** a release is published only when its artifact, test matrix, supported instruction list, limitations, and rollback path are documented.

## Immediate next execution order

1. Complete IR read/write/exception metadata and decoder negative tests.
2. Implement basic-block boundaries and interpreter-equivalent block execution.
3. Implement SSE/scalar lowering for the existing IR subset.
4. Connect the cache to executable-page generations and add self-modifying-code tests.
5. Expand per-thread Windows state and controlled exception chaining.
6. Add CPUID/XGETBV interception only after the preceding process gates pass.
7. Expand the Windows application fixture suite before testing real applications.
8. Start AVX2 only after AVX1 process-level differential coverage is green.

The Intel SDE launcher remains available as a clearly separate fallback, but its results never satisfy a native-backend milestone.
