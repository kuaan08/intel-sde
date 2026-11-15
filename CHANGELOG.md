# Changelog

## Unreleased — Independent runtime milestone

- Integrated decoded linear programs with shared IR and block construction.
- Added executable-region generation tracking for process/JIT invalidation groundwork.
- Added a Windows suspended-process host and experimental injected VEX128 VEH shim.
- Added a Windows AVX1 guest fixture target and an interpreter throughput benchmark.
- Integrated synthetic guest-memory execution into the independent AVX1 interpreter for supported loads, stores, ModRM/SIB, displacement, and RIP-relative forms.
- Documented the independent runtime as the primary backend and Intel SDE 9.58 as an isolated optional fallback.
- Clarified that the independent launcher backend runs a self-test only and does not launch game processes.
- Added an explicit external Intel SDE launch label and backward-compatible loading of older profile names.
- Added a Windows compatibility/DPI manifest so diagnostics identify Windows 10 correctly.
- Added a shared IR data model for vector operations and memory operands.
- Added an x64 ModRM/SIB address decoder with RIP-relative and displacement handling.
- Added a translated-block cache prototype with overlap-based invalidation.
- Added CPUID/XGETBV coverage policy derivation that keeps AVX hidden until memory coverage is enabled.
- Added deterministic AVX1 application-style regression programs, including VMOV store forms.
- Added Xenia-oriented profile templates and a Windows x64 release packaging script.
- Added GitHub Actions CI for Linux no-AVX/QEMU, Windows x64, CMake, the SSE POC, and the C# launcher.
- Fixed CRLF handling and strict hexadecimal validation in the runtime program parser.
- Rejected reserved VEX.X encodings in register-only VEX3 forms.
- Added register-register VMOV store-form decoding to align the runtime with the SSE POC.
- Made `--run-file` return success after any valid program, including an empty/comment-only program.
- Made the CMake build work on non-x86 hosts and made CTest runner arguments configurable.
- Hardened launcher profile serialization and filesystem error handling.
- Added an independent VEX AVX1 interpreter runtime.
- Added VEX.128 and VEX.256 register-only decoding.
- Added packed-single arithmetic, logical operations, register moves, and `VZEROUPPER`.
- Added sequential program-file execution and deterministic state digests.
- Added no-AVX cross-build and QEMU validation.
- Added Windows x64 CMake build script.
- Added a Windows WinForms launcher with profile management, diagnostics, logs, and backend selection.
- Made the independent runtime the launcher default backend.
- Kept Intel SDE 9.58 as an explicitly optional external comparison package.
- Added professional English project documentation and legal package boundaries.

## Earlier prototype work

- Added AVX1-to-legacy-SSE register-only proof of concept.
- Added VEX encoding fixtures and generated-code audits.
- Added Windows and Linux Intel SDE package organization and non-invasive audits.
