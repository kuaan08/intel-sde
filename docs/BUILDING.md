# Building and Testing

## Linux development environment

The canonical no-AVX test uses an x86-64 cross-compiler and QEMU when the development host is ARM64:

```bash
cd avx-runtime
./tests/run-tests.sh
```

The test performs three checks:

1. builds the runtime with AVX, AVX2, FMA, and AVX-512 disabled;
2. executes self-test, decoder test, and a sequential program under QEMU;
3. disassembles the binary with an x86-64 objdump and rejects V-prefixed AVX mnemonics.

The AVX1-to-SSE proof of concept has its own test suite:

```bash
cd avx1-sse-poc
./tests/run-tests.sh
```

## Windows development environment

Use Visual Studio or a compatible x64 generator:

```bat
cd avx-runtime
Build-Windows.bat

cd ..\sde-avx-lab-app
Build.bat
Start.bat
```

The Windows environment is the required environment for process-level compatibility work. The ARM64 sandbox can validate decoding and code-generation invariants, but it cannot prove Windows process interception or game compatibility.

## Test evidence

Every new instruction should include:

- independent encoding fixtures;
- decoder field assertions;
- scalar semantic reference;
- 128-bit and 256-bit cases;
- aliasing and upper-half state cases;
- generated-code disassembly;
- negative tests for unsupported forms.
