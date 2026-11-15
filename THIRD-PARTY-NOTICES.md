# Third-Party Notices

## SDE AVX Lab original code

Original source under `avx-runtime/`, `avx1-sse-poc/`, `sde-avx-lab-app/`, and project documentation is covered by the root `LICENSE`, unless a file states otherwise.

## Intel Software Development Emulator

The directory `intel-sde/` contains Intel Software Development Emulator distributions and related materials obtained from the Intel distribution referenced by the project. Those materials remain under their original Intel and third-party license terms. They are not relicensed by the root MIT license.

The project does not modify, reverse engineer, decompile, disassemble, or copy implementation code from the Intel SDE binaries. The package is retained as an optional external test and comparison reference.

Read the applicable files under:

```text
intel-sde/windows-9.58/Licenses/
intel-sde/linux-9.14/Licenses/
```

## Other dependencies

The project may use system toolchains such as CMake, GCC, Visual Studio, .NET Framework, QEMU, and Mono during development. Each toolchain remains governed by its own license and installation terms. Generated binaries should be redistributed only when the corresponding toolchain and dependency licenses permit it.
