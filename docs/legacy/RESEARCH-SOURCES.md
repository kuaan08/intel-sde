# Research Sources Catalog

> Historical research index. This file records the sources that informed the project architecture; it is not a list of bundled dependencies.

## Project scope and reference platform

SDE AVX Lab investigates an independent dynamic binary translator for x86-64 applications that require AVX on hosts without physical AVX support. The first target is Windows 10 x64 on an Intel Celeron N5100 with SSE4.2 and no listed AVX support. The project must distinguish verified facts, hypotheses, and proposals, and must not modify proprietary Intel SDE or Pin binaries.

## Dynamic instrumentation and translation

### DynamoRIO

- Repository: <https://github.com/DynamoRIO/dynamorio>
- Documentation: <https://dynamorio.org/>
- Code manipulation API: <https://dynamorio.org/API_BT.html>
- Deployment documentation: <https://dynamorio.org/page_deploy.html>
- License: <https://dynamorio.org/page_license.html>

Investigate Windows x64 instrumentation, block transformation, code-cache management, child-process tracking, executable-page changes, and JIT-generated code. DynamoRIO is a possible DBI foundation, not a ready-made AVX-to-SSE translator.

### QEMU TCG

- Repository: <https://github.com/qemu/qemu>
- TCG internals: <https://www.qemu.org/docs/master/devel/tcg.html>
- TCG plugins: <https://www.qemu.org/docs/master/devel/tcg-plugins.html>
- Emulation overview: <https://www.qemu.org/docs/master/about/emulation.html>
- User-mode documentation: <https://www.qemu.org/docs/master/user/main.html>
- License: <https://github.com/qemu/qemu/blob/master/LICENSE>

Study translation blocks, intermediate representation, host code generation, chaining, invalidation, CPU state, and exception handling. QEMU is a technical reference and a possible full-VM alternative, not a lightweight native Windows process shim. QEMU is GPL-2.0 overall and must be reviewed file by file before reuse.

### Intel Pin and SDE

- Pin documentation: <https://www.intel.com/content/www/us/en/developer/articles/tool/pin-a-dynamic-binary-instrumentation-tool.html>
- Pin downloads: <https://www.intel.com/content/www/us/en/developer/articles/tool/pin-a-binary-instrumentation-tool-downloads.html>
- SDE documentation: <https://www.intel.com/content/www/us/en/developer/articles/tool/software-development-emulator.html>
- Pin license: <https://software.intel.com/sites/landingpage/pintool/intel-simplified-software-license.txt>

Use Pin/SDE only as an external architectural or behavioral reference under the applicable license. Do not modify, reverse-engineer, or copy proprietary implementation code. The recorded laboratory result is that SDE 9.58.0 opened Xenia Canary with `-snb`; that did not establish gameplay compatibility.

## x86/VEX decoding and semantics

### Intel XED

- Repository: <https://github.com/intelxed/xed>
- Documentation: <https://intelxed.github.io/>
- MBuild: <https://github.com/intelxed/mbuild>
- License: <https://github.com/intelxed/xed/blob/main/LICENSE>

Investigate VEX decoding, operands, vector width, register numbering, addressing, and encoding. Confirm licenses for XED, MBuild, and every distributed artifact before integration.

### Intel architecture references

- Intel Software Developer Manuals: <https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html>
- Intel Intrinsics Guide: <https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html>

The Intel SDM is the normative source for instruction semantics, VEX encoding, MXCSR, exceptions, XMM/YMM transitions, XSAVE/XRSTOR, CPUID, and XGETBV. The Intrinsics Guide is a useful mapping aid but does not replace the architectural manuals.

## Application and hardware references

### Xenia

- Xenia Canary: <https://github.com/xenia-canary/xenia-canary>
- Canary quickstart: <https://github.com/xenia-canary/xenia-canary/wiki/Quickstart>
- Canary FAQ: <https://github.com/xenia-canary/xenia-canary/wiki/FAQ>
- Original Xenia: <https://github.com/xenia-project/xenia>
- Original quickstart: <https://github.com/xenia-project/xenia/wiki/Quickstart>

Investigate CPU feature checks, the x64 JIT backend, generated code, initialization, logging, child processes, and per-game behavior. A bypassed CPUID check does not provide the missing instruction implementation.

### Intel Celeron N5100

- Official specifications: <https://www.intel.com/content/www/us/en/products/sku/212329/intel-celeron-processor-n5100-4m-cache-up-to-2-80-ghz/specifications.html>

Use the official specification as the authority for the reference processor's listed instruction extensions, cores, frequency, and memory capabilities.

## Complementary engineering references

- QBDI API: <https://qbdi.readthedocs.io/en/stable/api_description.html>
- QBDI license: <https://github.com/QBDI/QBDI/blob/dev-next/LICENSE.txt>
- Frida Stalker: <https://frida.re/docs/stalker/>
- Frida Gum license: <https://github.com/frida/frida-gum/blob/main/COPYING>
- LLVM ORC: <https://llvm.org/docs/ORCv2.html>
- LLVM license: <https://llvm.org/LICENSE.txt>
- CMake documentation: <https://cmake.org/documentation/>
- LLVM project: <https://github.com/llvm/llvm-project>

These projects are candidates for controlled experiments only. Their licenses, dependencies, and suitability for Windows x64 process translation must be evaluated before adoption.

## Research and validation rules

1. Read the primary documentation and license for every candidate component.
2. Separate verified facts, inferences, and proposals in technical reports.
3. Test each AVX instruction against an independent scalar reference and, when available, AVX-capable hardware.
4. Cover NaNs, infinities, subnormals, signed zero, rounding, MXCSR, alignment, aliasing, memory faults, threads, and exceptions.
5. Audit generated code to ensure that the host backend does not emit unsupported V-prefixed instructions.
6. Do not use opening a game window or an FPS observation as the first correctness test.
7. Do not expose AVX or AVX2 through CPUID until the corresponding execution coverage has been validated.
