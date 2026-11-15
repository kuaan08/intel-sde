# SDE AVX Lab

## Independent AVX Compatibility Runtime for Windows x64

[![CI](https://github.com/kuaan08/intel-sde/actions/workflows/ci.yml/badge.svg)](https://github.com/kuaan08/intel-sde/actions/workflows/ci.yml)

SDE AVX Lab is an independent research and engineering project for running selected x86-64 applications that require Intel Advanced Vector Extensions on host processors without physical AVX support.

The project is designed for Windows 10/11 x64 systems, including low-power machines such as the Intel Celeron N5100. Its long-term goal is to provide an independent dynamic binary translation runtime that can decode incompatible VEX instructions, preserve architectural state, and execute equivalent SSE4.2-compatible code on the host.

This repository contains original project code, experimental runtime components, test programs, documentation, and a Windows launcher. The Intel Software Development Emulator package is retained separately as an optional external comparison tool. It is not the implementation of this project.

## Current status

The project is in active prototype development.

The current independent runtime can:

- decode a controlled subset of VEX-encoded AVX1 instructions;
- interpret VEX.128 and VEX.256 register-only operations;
- emulate `VADDPS`, `VSUBPS`, `VMULPS`, and `VDIVPS`;
- emulate `VXORPS`, `VANDPS`, and `VORPS`;
- emulate register-register `VMOVUPS` and `VMOVAPS`;
- emulate `VZEROUPPER`;
- maintain 16 logical YMM registers;
- execute instruction programs stored in text files;
- produce deterministic state checksums;
- run tests compiled with AVX disabled on the host toolchain.

The current runtime is not yet a full process-level DBT and must not be advertised as a finished game emulator. Memory operands are currently decoded into a shared IR prototype, while execution of guest memory and process interception are still under development. A block-cache and coverage-gating prototype is present; JIT code support, Windows exception integration, and broad AVX/AVX2 coverage are not complete.

## Repository layout

```text
sde-gaming-4f6209bb/
├── avx-runtime/                 Independent AVX1 interpreter runtime
│   ├── src/                     Runtime source code
│   ├── include/                 Shared IR, memory addressing, cache, and feature policy
│   ├── tests/                   Runtime programs and automated tests
│   ├── CMakeLists.txt            Cross-platform build definition
│   └── Build-Windows.bat         Windows x64 build script
├── avx1-sse-poc/                AVX1-to-legacy-SSE proof of concept
├── sde-avx-lab-app/             Windows WinForms launcher source
├── windows-9.58/                Optional external Intel SDE Windows package
├── linux-9.14/                  Historical Linux reference package
├── lab/                         Non-invasive package audit tools
├── docs/                        Project architecture and engineering notes
├── profiles/                    Application profile templates
├── tools/                       Windows release packaging scripts
├── LICENSE                      License for original SDE AVX Lab files
└── THIRD-PARTY-NOTICES.md       External package and license boundaries
```

## Build the independent runtime on Windows

Requirements:

- Windows 10 or Windows 11 x64;
- CMake 3.16 or newer;
- Visual Studio 2019/2022 with the Desktop C++ workload, or another compatible x64 C++ toolchain.

Build and run the self-test:

```bat
cd avx-runtime
Build-Windows.bat
```

The expected output includes:

```text
PASS: independent AVX1 interpreter self-test; no AVX host instructions required.
```

The runtime can also execute the checked-in instruction program:

```bat
build-windows\Release\avx_runtime.exe --run-file tests\sample-program.txt
```

Decode a VEX instruction without executing it:

```bat
build-windows\Release\avx_runtime.exe --decode "c4 41 34 58 e6"
```

## Build the Windows application

The launcher is original project code. It provides a graphical workflow for selecting an application, storing per-game profiles, selecting a backend, writing diagnostics, and launching the optional external SDE backend.

Build it on Windows with .NET Framework 4.x:

```bat
cd sde-avx-lab-app
Build.bat
Start.bat
```

The application selects the SDE AVX Lab Runtime as its default backend. In the current prototype, that backend runs only the independent runtime self-test and does not launch an arbitrary game process. To launch Xenia or another target, select the explicitly labelled Intel SDE 9.58 external backend.

## Independent runtime test on Linux x86-64 or ARM64/QEMU

On an x86-64 Linux host:

```bash
cd avx-runtime
./tests/run-tests.sh
```

On the ARM64 development sandbox, the test script uses an x86-64 cross-compiler and QEMU user-mode. It disables AVX, AVX2, FMA, and AVX-512 code generation and verifies that the generated runtime contains no V-prefixed AVX instructions.

## Continuous integration

GitHub Actions runs `.github/workflows/ci.yml` on pushes and pull requests. The Linux job uses the x86-64 cross-compiler and QEMU to test the interpreter and AVX1-to-SSE proof of concept without AVX. The Windows job builds the independent runtime, the SSE POC, and the WinForms launcher with x64 toolchains. Generated build trees are excluded from version control.

## Example instruction program

`avx-runtime/tests/sample-program.txt` contains a small independent program:

```text
c5 f0 58 c2
c5 e4 57 db
c5 f8 77
```

The runtime parses one VEX instruction per line, ignores comments, executes the sequence, and prints a state digest. This format is intentionally simple and deterministic so that decoder and emulator behavior can be tested before process-level DBT is introduced.

## Architecture

The project is being developed in layers:

1. **Instruction decoder** — parses VEX prefixes, opcode fields, register operands, vector width, and reserved fields.
2. **Independent interpreter** — provides a correctness-first fallback for the supported subset.
3. **SSE lowering backend** — emits legacy SSE4.2-compatible sequences for supported AVX1 operations.
4. **Intermediate representation** — unifies interpretation and compiled translation.
5. **Basic-block translator** — translates complete blocks while preserving control flow.
6. **Code cache** — caches translated blocks and invalidates them when executable memory changes.
7. **Windows process runtime** — adds thread, exception, loader, JIT, CPUID, and XGETBV handling.
8. **Game compatibility layer** — adds per-application profiles, diagnostics, telemetry, and safe fallbacks.

The runtime must never advertise AVX or AVX2 through CPUID/XGETBV until the corresponding execution coverage is implemented and tested.

The public Intel SDE capability comparison and independent implementation plan are documented in [docs/INTEL-SDE-FEATURE-MAP.md](docs/INTEL-SDE-FEATURE-MAP.md).

## Planned instruction coverage

The first correctness target is AVX1 packed single-precision arithmetic and data movement:

- `VADDPS`
- `VSUBPS`
- `VMULPS`
- `VDIVPS`
- `VMOVUPS`
- `VMOVAPS`
- `VXORPS`
- `VANDPS`
- `VORPS`
- `VZEROUPPER`

The next coverage milestones are memory operands, scalar forms, integer AVX operations, conversion instructions, FMA policy, and AVX2 integer/vector operations. Every new instruction requires semantic tests and a host-code audit.

## Validation policy

A feature is not considered complete because a program starts. Each backend change must be tested against:

- an independent scalar reference;
- a legacy SSE implementation where applicable;
- a compatible AVX execution reference when available;
- NaN, infinity, signed zero, rounding, overflow, underflow, and aliasing cases;
- 128-bit and 256-bit state transitions;
- generated-code disassembly confirming that the no-AVX backend does not contain AVX instructions;
- cache, thread, and exception behavior once process-level translation exists.

Opening a game window is not evidence of playable compatibility. Results must record initialization, menu, loaded content, frame rate, crashes, visual correctness, and logs separately.

## Optional Intel SDE reference package

The `windows-9.58` and `linux-9.14` directories are not the project runtime. They are kept intact for authorized external comparison and documentation review.

The Intel packages are governed by their own license terms. They are not modified, patched, reverse engineered, or used as a source of copied implementation code. See `THIRD-PARTY-NOTICES.md` and `lab/README.md`.

## Legal and engineering boundary

SDE AVX Lab is an independent implementation. It may learn from public ISA documentation, public compiler behavior, open-source projects with compatible licenses, and externally observed test results. It must not copy proprietary implementation code or derive code through prohibited reverse engineering of Intel binaries.

The original SDE AVX Lab source code is licensed under the license in `LICENSE`. That license does not relicense or alter any third-party package stored under `intel-sde/`.

## Roadmap

- [x] Independent VEX decoder prototype
- [x] AVX1 register-only interpreter prototype
- [x] AVX1-to-SSE register-only proof of concept
- [x] Windows launcher prototype
- [x] ModRM/SIB memory operands and x64 addressing decoder prototype
- [x] Shared IR data model and block-cache invalidation prototype
- [ ] Basic-block translation
- [ ] Executable-memory discovery and process-level invalidation
- [ ] Windows thread and exception runtime
- [x] CPUID/XGETBV coverage policy prototype
- [x] Deterministic AVX1 application-style regression suite
- [ ] AVX2 implementation milestones
- [x] Game-specific compatibility profile templates
- [x] Windows CI build and release packaging
- [ ] Performance tuning and benchmark suite

## Contributing

Keep changes small and testable. Add a decoder fixture before adding a new opcode. Add semantic tests before adding an optimization. Do not mix external Intel package files with original runtime source files. Every benchmark must identify the host CPU, compiler, build flags, workload, and whether the result came from the independent runtime or an external reference backend.
