# Changelog

## Unreleased — Independent runtime milestone

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
