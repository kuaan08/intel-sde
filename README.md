# SDE AVX Lab

**Independent AVX/AVX2 compatibility runtime for Windows x64**
Dynamic binary translation research · VEX decoding · SSE4.2 fallback · Windows 10/11

[![CI](https://github.com/kuaan08/intel-sde/actions/workflows/ci.yml/badge.svg)](https://github.com/kuaan08/intel-sde/actions/workflows/ci.yml)
[![Latest commit](https://img.shields.io/github/last-commit/kuaan08/intel-sde)](https://github.com/kuaan08/intel-sde/commits/master)

> **Work in progress — not finished.** The independent runtime contains a controlled AVX1 interpreter, shared IR metadata, bounded basic-block translation, a register-register legacy-SSE lowering prototype, executable-page tracking primitives, per-thread guest-state primitives, diagnostics, and a coverage-gated CPUID/XGETBV policy model. The process-level Windows DBT gate, complete memory/helper lowering, loader/JIT integration, SEH/VEH chaining, broad AVX1/AVX2 coverage, and application compatibility are not complete.
>
> The Intel SDE package remains available only as a separate fallback for comparison and temporary compatibility testing. It is not the implementation of this project.

**Contents:** Status · Progress · Validation boundary · Builds and downloads · Quick start · Architecture · Runtime scope · Windows launcher · Documentation · Legal boundary · License

## Status and validation boundary

The repository is an engineering prototype, not a compatibility release. “Implemented” below means that source code and deterministic core tests exist; it does **not** mean that a Windows process can execute arbitrary AVX code. Process-level gates remain closed until the translated backend, state capture, memory faults, loader/JIT lifecycle, exception chaining, and feature virtualization are validated together.

The currently recorded validation was performed with an x86-64 cross-compiler and QEMU user-mode configured to avoid AVX host code generation. No Windows 10/11 physical machine or Windows virtual machine was used to validate this latest repository state. The Windows build and Windows-only process path therefore remain **not validated in this environment**. No compatibility claim is made for Xenia, games, or arbitrary Windows applications.

## Progress

The project is focused on building an independent dynamic binary translation runtime for x86-64 applications that require AVX on processors without physical AVX support.

The current independent core can:

- decode a controlled subset of VEX-encoded AVX1 instructions;
- execute VEX.128 and VEX.256 packed-single operations without AVX host instructions;
- emulate `VADDPS`, `VSUBPS`, `VMULPS`, `VDIVPS`, `VXORPS`, `VANDPS`, and `VORPS`;
- emulate `VMOVUPS`, `VMOVAPS`, and `VZEROUPPER`;
- maintain 16 logical YMM registers with correct 128-bit upper-lane clearing;
- decode ModRM/SIB, displacements, and RIP-relative x64 addressing;
- execute supported synthetic guest-memory loads and stores;
- maintain deterministic guest GPR, vector, and memory state for regression tests;
- expose shared IR, feature-coverage policy, and translated-block cache prototypes;
- build a Windows process host and an experimental injected VEH shim for a small AVX1 VEX.128 register-and-memory subset;
- compile a Windows AVX1 guest application fixture without executing it on a no-AVX host;
- report deterministic interpreter throughput for regression tracking;
- provide a Windows x64 build path and validate the no-AVX core through an x86-64 target under QEMU; Windows process execution remains unvalidated here;
- verify that generated no-AVX binaries do not contain unsupported V-prefixed host instructions.

These capabilities are core validation stages. They do not yet mean that an arbitrary Windows game or application can be translated and executed by the independent backend.

### Current engineering boundary

The independent runtime is the primary backend and receives all new implementation work. The core now contains bounded block translation, control-flow classification, shared-IR legacy-SSE emission for register-register operations, executable-region generation tracking, a composite block-cache key, synchronized guest thread-state storage, structured unsupported-instruction reports, and deterministic feature-policy responses. The process path still handles only a narrow experimental VEX.128 VEH subset; it does not yet connect those core primitives into a complete Windows DBT dispatcher.

The external Intel SDE backend is intentionally kept separate. It is useful for collecting comparison data or launching a target while the independent process runtime is being completed, but an application running under Intel SDE is not evidence that the SDE AVX Lab backend works.

## Builds and downloads

GitHub Actions compiles and tests the project on every push and pull request.

- [Latest CI workflow](https://github.com/kuaan08/intel-sde/actions/workflows/ci.yml)
- [All workflow runs](https://github.com/kuaan08/intel-sde/actions)
- Windows x64 packages are available as artifacts from successful CI runs.

The Windows artifact contains the independent runtime, the AVX1-to-SSE proof of concept, the WinForms launcher, documentation, license notices, and SHA-256 checksums. Intel SDE is not redistributed by this project.

## Quick start

### Independent runtime

On Windows x64, extract the CI package and run:

```bat
avx_runtime.exe --self-test
```

Expected result:

```text
PASS: independent AVX1 interpreter self-test; register and memory operands; no AVX host instructions required.
```

Decode a VEX instruction without executing it:

```bat
avx_runtime.exe --decode "c5 f0 58 00"
```

The decoder reports the instruction as a 128-bit `VADDPS` with an `[rax]` memory source.

Run the checked-in deterministic program suite after building from source:

```bat
avx_runtime.exe --run-file tests\programs\memory-avx1.txt
```

### Build from source on Windows

Requirements:

- Windows 10 or Windows 11 x64;
- CMake 3.16 or newer;
- Visual Studio 2019/2022 with the Desktop C++ workload, or a compatible x64 C++ toolchain.

```bat
cd avx-runtime
Build-Windows.bat
build-windows\Release\avx_runtime.exe --self-test
```

### Build and test on Linux or ARM64/QEMU

On an x86-64 Linux host:

```bash
cd avx-runtime
./tests/run-tests.sh
./tests/run-avx1-suite.sh
```

On the ARM64 development environment, the scripts use an x86-64 cross-compiler and QEMU user-mode. The build disables AVX, AVX2, FMA, and AVX-512 code generation.

For a CMake/CTest build with an explicit x86-64 runner:

```bash
cmake -S avx-runtime -B avx-runtime/build/test \
  -DCMAKE_CXX_COMPILER=x86_64-linux-gnu-g++ \
  -DSDE_AVX_TEST_RUNNER=/usr/bin/qemu-x86_64 \
  -DSDE_AVX_TEST_RUNNER_ARGS="-L /usr/x86_64-linux-gnu"
cmake --build avx-runtime/build/test --parallel
ctest --test-dir avx-runtime/build/test --output-on-failure
```

## Architecture

The runtime is being developed in independent layers so that each layer can be tested before process integration.

```text
Windows x64 application
          │
          ▼
Executable-page discovery and thread runtime
          │
          ▼
VEX decoder ──► shared IR ──► basic-block translator
                                  │
                                  ▼
                         translated-block cache
                                  │
                    ┌─────────────┴─────────────┐
                    ▼                           ▼
             scalar interpreter             SSE4.2 backend
                    │                           │
                    └─────────────┬─────────────┘
                                  ▼
                    host CPU without physical AVX
```

The planned process runtime must preserve architectural state, memory ordering, MXCSR behavior, upper YMM state, control flow, thread-local state, exceptions, and self-modifying/JIT code invalidation. CPUID and XGETBV must remain coverage-gated; the runtime must never advertise AVX or AVX2 before the corresponding execution path is implemented and tested.

## Runtime scope

The first correctness target is AVX1 packed single-precision data movement and arithmetic:

```text
VADDPS       VSUBPS       VMULPS       VDIVPS
VMOVUPS      VMOVAPS      VXORPS       VANDPS
VORPS        VZEROUPPER
```

The current memory harness uses deterministic guest registers and memory only for core validation. It is not a substitute for reading and writing another Windows process. Process-level memory access will be implemented with explicit range checks, page handling, executable-page tracking, and invalidation tests.

The next instruction milestones are scalar forms, integer AVX operations, conversions, broadcasts, permutations, and AVX2 integer/vector operations. Each instruction requires semantic tests, differential checks, and a host-code audit.

## Windows launcher

The repository includes an original WinForms launcher for selecting the backend, target executable, working directory, CPU profile, diagnostics, and per-application settings.

The default entry is:

```text
SDE AVX Lab Runtime (principal; experimental)
```

At the current stage it runs the independent runtime self-test and core experiments. It does not yet translate an arbitrary game process.

The separate fallback entry is:

```text
Intel SDE 9.58 (fallback externo)
```

It is retained for users who need a temporary external compatibility path or comparison data. To start the launcher from `cmd.exe`, quote the filename because it contains spaces:

```bat
start "SDE AVX Lab" "SDE AVX Lab.exe"
```

Build the launcher with .NET Framework 4.x:

```bat
cd sde-avx-lab-app
Build.bat
Start.bat
```

## Repository layout

```text
avx-runtime/                 Independent AVX1 runtime and core tests
avx1-sse-poc/                AVX1-to-legacy-SSE proof of concept
sde-avx-lab-app/             Original Windows launcher source
profiles/                    Application profile templates
docs/                        Architecture, backend strategy, and milestones
tools/                       Windows release packaging scripts
windows-9.58/                Optional external Intel SDE fallback package
linux-9.14/                  Historical reference package
lab/                         Non-invasive external-package audit tools
.github/workflows/           Reproducible Linux and Windows CI
LICENSE                      License for original SDE AVX Lab files
THIRD-PARTY-NOTICES.md       Third-party package and license boundaries
```

## Documentation

- [Architecture](docs/ARCHITECTURE.md)
- [Backend strategy](docs/BACKEND-STRATEGY.md)
- [Next engineering milestones](docs/NEXT-MILESTONES.md)
- [Windows process runtime](docs/WINDOWS-PROCESS-RUNTIME.md)
- [Intel SDE public feature map](docs/INTEL-SDE-FEATURE-MAP.md)
- [Build guide](docs/BUILDING.md)
- [Runtime documentation](avx-runtime/README.md)
- [Launcher documentation](sde-avx-lab-app/README.md)
- [Third-party notices](THIRD-PARTY-NOTICES.md)

## Validation policy

A feature is not considered complete because a process starts. Every backend change must be tested against an independent scalar reference and, where applicable, a legacy SSE implementation or a compatible AVX reference.

Tests must cover 128-bit and 256-bit transitions, aliasing, memory boundaries, NaN and infinity behavior, signed zero, rounding, overflow, underflow, MXCSR-sensitive behavior, generated-code disassembly, cache invalidation, threads, and exceptions as those layers become available.

Opening a game window is not evidence of playable compatibility. Compatibility claims require reproducible logs, initialization results, loaded content, frame-time measurements, crash behavior, and correctness observations from the independent backend.

## Legal and engineering boundary

SDE AVX Lab is an independent implementation. It may use public ISA documentation, public compiler behavior, open-source projects with compatible licenses, and externally observed test results.

The Intel SDE package is not modified, patched, or used as a source of copied implementation code. Its files remain subject to their own license terms. See [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md) and [lab/README.md](lab/README.md).

Use only software and application files that you are authorized to use. No games, keys, firmware, dumps, proprietary Intel implementation code, or third-party assets are distributed by this repository.

## Roadmap

The roadmap is divided into executable phases with dependencies and acceptance gates in [Next Engineering Milestones](docs/NEXT-MILESTONES.md). The compact status below is intentionally conservative: a prototype is not marked complete when its process-level acceptance gate is still missing.

### Completed foundations

- [x] Independent VEX decoder prototype
- [x] AVX1 register and synthetic-memory interpreter subset
- [x] VEX.128/VEX.256 logical vector state and upper-lane rules
- [x] ModRM/SIB, displacement, and RIP-relative x64 address decoding
- [x] Shared IR data model and linear-program integration
- [x] Coverage-gated feature policy prototype
- [x] Executable-region and write-generation cache prototype
- [x] Deterministic AVX1 regression programs and Windows VEX fixture
- [x] Windows CI, dual launchers, and release packaging
- [x] Interpreter throughput benchmark prototype

### Implemented foundations — acceptance gates still open

- [x] IR read/write-set, memory width, alignment, and exception metadata primitives
- [x] Decoder negative tests for the currently supported VEX contract
- [x] Bounded basic-block translation and control-flow classification primitives
- [x] Register-register legacy-SSE lowering from shared IR
- [x] Executable-region discovery primitive and generation-based cache invalidation
- [x] Synchronized per-thread state and structured diagnostics primitives
- [x] Coverage-gated virtual CPUID/XGETBV response model

The acceptance gates for these foundations remain open where process integration, memory helpers, complete exits, loader/JIT handling, SEH/VEH chaining, or Windows validation are still missing.

### Planned after the process gates

- [ ] AVX1 Windows application test suite
- [ ] Differential AVX-capable reference matrix
- [ ] AVX2 decoder, interpreter, lowering, and coverage milestones
- [ ] Hot-block optimization and complete benchmark suite
- [ ] Versioned profiles, diagnostic bundles, and compatibility release workflow

## Acknowledgements

The project studies public documentation and open-source runtime techniques from the x86 ecosystem, including decoder, instrumentation, and dynamic translation concepts. Those references inform architecture and testing only; the independent runtime source remains original project code.

## License

Original SDE AVX Lab source files are distributed under [LICENSE](LICENSE). Third-party packages and their licenses are described separately in [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).
