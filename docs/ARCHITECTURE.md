# SDE AVX Lab Architecture

## Product boundary

The product is an independent Windows x64 compatibility runtime and game-oriented management application. The application is responsible for profiles, launch configuration, logs, diagnostics, and compatibility policy. The runtime is responsible for instruction decoding, state management, interpretation, translation, and process execution.

The external Intel SDE package is never a required runtime dependency for the independent product.

## Guest and host model

The guest is an x86-64 process whose instruction stream may contain VEX AVX instructions. The initial host target is an x86-64 CPU with SSE4.2 and without AVX. The runtime must preserve guest architectural behavior while emitting only host-supported code.

The runtime must maintain a guest vector state containing the lower and upper halves of logical YMM registers. A VEX.128 instruction writes the lower XMM half and clears the corresponding upper half. A VEX.256 instruction writes both halves. `VZEROUPPER` clears all upper halves while preserving lower halves.

## Execution tiers

### Tier 0: independent interpreter

The interpreter is the correctness fallback. It consumes decoded instructions and updates a guest state model. It is slow but easy to test and can serve as a reference for compiled translations.

### Tier 1: SSE block compiler

The compiler lowers supported instructions into legacy SSE sequences. It uses a temporary state layout while correctness is established. Later versions may keep hot values in host XMM registers, but only after dependency and exception behavior is tested.

### Tier 2: dynamic block runtime

The DBT runtime discovers executable blocks, decodes instructions until a control-flow boundary, compiles or interprets the block, and redirects execution to the translated block. It needs page permissions, code invalidation, thread-local state, exception mapping, and safe handling of indirect branches.

### Tier 3: application compatibility layer

The application layer stores per-game policies, selects the minimum required guest feature profile, collects logs and telemetry, and provides safe fallback behavior. It must not advertise an instruction set that the runtime cannot execute.

## Required invariants

- no AVX instruction may be emitted by the no-AVX backend;
- unsupported instructions must fail closed or use a tested interpreter fallback;
- CPUID and XGETBV must agree with actual execution coverage;
- 128-bit and 256-bit upper-half semantics must be preserved;
- self-modifying/JIT code must invalidate affected translated blocks;
- exceptions and MXCSR behavior must not be silently discarded;
- every optimization must have an equivalent correctness test.

## Development sequence

1. Expand decoder and fixtures.
2. Add memory operands and address calculation.
3. Build a common IR.
4. Add interpreter execution for the IR.
5. Add SSE lowering for the same IR.
6. Add block cache and invalidation.
7. Integrate Windows process execution.
8. Add CPUID/XGETBV policy gating.
9. Measure applications and games only after controlled tests pass.
