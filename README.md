# SDE AVX Lab

**Independent AVX/AVX2 compatibility-runtime research for Windows x64**

Dynamic binary translation research · VEX decoding · SSE4.2/scalar lowering · Windows process integration

[![CI](https://github.com/kuaan08/intel-sde/actions/workflows/ci.yml/badge.svg)](https://github.com/kuaan08/intel-sde/actions/workflows/ci.yml)
[![Latest commit](https://img.shields.io/github/last-commit/kuaan08/intel-sde)](https://github.com/kuaan08/intel-sde/commits/master)

> **Project status: engineering prototype.** The repository contains a controlled AVX1 interpreter, shared IR metadata, bounded block-translation primitives, legacy-SSE lowering prototypes, executable-page tracking, per-thread state, diagnostics, a Windows VEH fixture, and a coverage-gated CPU-feature model. It is not yet a complete process-level DBT and does not claim compatibility with Xenia, games, or arbitrary Windows applications.

## Project goal

SDE AVX Lab investigates an independent runtime that can execute selected x86-64 AVX instructions on hosts without physical AVX support. The initial reference platform is Windows 10 x64 on an Intel Celeron N5100 with SSE4.2 and no listed AVX support.

The project is not a launcher-only CPUID workaround. The long-term goal is a functional translator that decodes VEX instructions, preserves architectural state, lowers supported operations to host-compatible SSE/scalar sequences, manages translated-block caching, and handles dynamically generated code.

Intel SDE 9.58 remains an isolated external comparison path. Its ability to start a target is not evidence that this independent runtime works, and no Intel SDE implementation code is used here.

## Current status

The independent core currently provides:

- a controlled VEX decoder for selected AVX1 packed and scalar-single forms;
- VEX.128 and VEX.256 logical vector state with upper-lane rules;
- scalar interpretation without requiring AVX host instructions;
- ModRM/SIB, displacement, RIP-relative addressing, and selected guest-memory forms;
- shared IR with read/write, memory-width, alignment, and exception metadata primitives;
- bounded basic-block construction and control-flow exit classification;
- a translated-block cache prototype with source-byte, module, feature-profile, and page-generation identity;
- legacy-SSE lowering for supported register-register IR operations;
- executable-page generation tracking, per-thread guest state, and structured diagnostics;
- an experimental Windows suspended-process host and injected VEH shim;
- packed and scalar VEX fixture validation through Wine/QEMU with a Conroe no-AVX CPU model;
- an opt-in main-image CPUID/XGETBV patch path, still pending native Windows and loader/JIT validation.
- a versioned CPUID/XGETBV fixture that passes under Wine/QEMU Conroe and a Xenia Canary liveness run that passes beyond the feature-query breakpoint stage; this is not yet a JIT translation or game-compatibility result.

These are implementation foundations, not a promise of arbitrary application compatibility. The complete process DBT path, memory helpers, control-flow execution, loader/JIT integration, SEH/VEH chaining, broad AVX1 coverage, AVX2 coverage, and application testing remain open.

## Validation boundary

The latest recorded validation was performed in the Linux ARM64 development environment using x86-64 cross-compilers and QEMU user-mode. The core was built with AVX, AVX2, FMA, and AVX-512 generation disabled. CMake, CTest, the scalar self-test, MinGW compilation, and the Windows VEX fixture passed.

No physical Windows 10/11 machine or Windows virtual machine has validated the latest repository state. The Windows process path is therefore still experimental. Opening a process window is not evidence of correct execution, playable performance, or game compatibility.

## Quick start

### Core build on Linux

```bash
cmake -S avx-runtime -B avx-runtime/build
cmake --build avx-runtime/build --parallel
ctest --test-dir avx-runtime/build --output-on-failure
./avx-runtime/build/avx_runtime --self-test
```

The deterministic program harness can decode or execute a checked-in byte program:

```bash
./avx-runtime/build/avx_runtime --decode "c5 f0 58 00"
./avx-runtime/build/avx_runtime --run-file avx-runtime/tests/programs/memory-avx1.txt
```

### Cross-validation without AVX host code generation

```bash
cd avx-runtime
./tests/run-tests.sh
./tests/run-avx1-suite.sh
```

The cross-validation scripts use an x86-64 compiler and QEMU user-mode where required. They audit generated code to ensure that unsupported V-prefixed host instructions are not emitted.

### Windows build

Requirements are Windows 10/11 x64, CMake 3.16 or newer, and Visual Studio 2019/2022 or a compatible x64 C++ toolchain.

```bat
cd avx-runtime
Build-Windows.bat
build-windows\Release\avx_runtime.exe --self-test
```

The experimental CPUID/XGETBV patch path is disabled by default. It can be selected explicitly for diagnostics with:

```bat
set SDE_AVX_LAB_EXPERIMENTAL_CPUID=1
```

This opt-in path currently targets only the main executable image. It is not a complete feature-virtualization implementation.

## Architecture

```text
Windows x64 application
          │
          ▼
Executable-page and thread runtime
          │
          ▼
VEX decoder ──► shared semantic IR ──► block translator
                                             │
                                             ▼
                                      translated-block cache
                                             │
                              ┌──────────────┴──────────────┐
                              ▼                             ▼
                    scalar interpreter              SSE4.2 lowering
                              │                             │
                              └──────────────┬──────────────┘
                                             ▼
                                  host CPU without physical AVX
```

The runtime must eventually preserve vector state, memory ordering, MXCSR behavior, upper YMM state, control flow, threads, exceptions, and self-modifying/JIT code invalidation. CPU feature reporting must remain gated by actual instruction coverage.

## Repository layout

| Path | Description |
|---|---|
| `avx-runtime/` | Independent runtime, decoder, IR, translator primitives, tests, and Windows host code. |
| `avx1-sse-poc/` | Offline AVX1-to-legacy-SSE proof of concept. |
| `sde-avx-lab-app/` | Original Windows launcher source and profile management. |
| `profiles/` | Application profile templates. |
| `docs/` | Maintained architecture, build, milestone, legal, and compatibility documentation. |
| `docs/legacy/` | Historical research and feasibility records retained for traceability. |
| `lab/` | Non-invasive audit tools for external packages. |
| `tools/` | Windows packaging and release scripts. |
| `windows-9.58/` | Optional external Intel SDE package; governed by its own license. |
| `linux-9.14/` | Historical reference package; not part of the independent backend. |
| `.github/workflows/` | Linux and Windows CI definitions. |

The vendor package directories contain upstream license files, manifests, generated data, and original documentation. Those files are intentionally preserved and are not subject to the project's English documentation convention.

## Documentation

The [documentation index](docs/README.md) is the canonical entry point.

- [Architecture](docs/ARCHITECTURE.md)
- [Backend strategy](docs/BACKEND-STRATEGY.md)
- [Building and testing](docs/BUILDING.md)
- [Next engineering milestones](docs/NEXT-MILESTONES.md)
- [Windows process runtime](docs/WINDOWS-PROCESS-RUNTIME.md)
- [Intel SDE feature map](docs/INTEL-SDE-FEATURE-MAP.md)
- [Xenia CPU feature study](docs/XENIA-CPU-FEATURE-STUDY-2026-11-15.md)
- [Xenia AVX interceptor test](docs/XENIA-AVX-INTERCEPTOR-TEST-2026-11-15.md)
- [Legal boundary and optimization plan](docs/LEGAL-BOUNDARY-AND-OPTIMIZATION-PLAN.md)
- [Runtime README](avx-runtime/README.md)
- [AVX1-to-SSE POC README](avx1-sse-poc/README.md)
- [Windows launcher README](sde-avx-lab-app/README.md)
- [Third-party notices](THIRD-PARTY-NOTICES.md)
- [Changelog](CHANGELOG.md)

## Windows launcher

The repository includes an original WinForms launcher for selecting executables, working directories, profiles, diagnostics, and backend choice. The independent runtime is the primary development backend. Intel SDE 9.58 is kept separate as an optional external comparison backend and is never silently downloaded or modified.

Build and start the launcher on Windows:

```bat
cd sde-avx-lab-app
Build.bat
Start.bat
```

The launcher does not turn the current prototype into a complete game-compatibility layer. Unsupported instructions may still terminate the target.

## Roadmap

The roadmap and acceptance gates are maintained in [Next Engineering Milestones](docs/NEXT-MILESTONES.md). The next major gates are process-level DBT dispatch, complete memory and control-flow handling, loader/JIT discovery, SEH/VEH chaining, differential AVX1 application tests, and only then broader AVX2 work and performance optimization.

## Legal and engineering boundary

This is an independent implementation. It may use public ISA documentation, public behavior observations, and compatible open-source projects after license review. The repository does not modify, patch, reverse-engineer, or copy implementation code from Intel SDE, Pin, or other proprietary packages.

Use only software and application files that you are authorized to use. No games, keys, firmware, dumps, proprietary Intel implementation code, or third-party assets are distributed by this repository. See [Legal Boundary and Independent Optimization Plan](docs/LEGAL-BOUNDARY-AND-OPTIMIZATION-PLAN.md) and [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).

## License

Original SDE AVX Lab source files are distributed under [LICENSE](LICENSE). Third-party packages and their licenses are described separately in [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).
