# SDE AVX Lab — AVX1-to-SSE Proof of Concept

**Status:** experimental correctness proof of concept; not an application compatibility runtime.

This prototype accepts one VEX instruction, decodes a deliberately narrow AVX1 form, and emits x86-64 bytes that use legacy SSE instructions only. A test state stores 16 logical YMM registers as pairs of 128-bit halves. The generated code uses XMM temporaries, performs the legacy SSE operation, and writes the result back to the logical state.

## Implemented subset

The current prototype covers packed-single `VADDPS`, `VSUBPS`, `VMULPS`, `VDIVPS`, `VXORPS`, `VANDPS`, and `VORPS` in VEX.128 and VEX.256 register-register forms. It also covers register-register `VMOVUPS` and `VMOVAPS`, `VZEROUPPER`, VEX2/VEX3 prefixes, and vector register numbers 8–15 in the accepted forms.

Unsupported forms are rejected explicitly. AVX2, memory operands, scalar instructions, branches, CPUID virtualization, XGETBV/XSAVE, architectural flags and exceptions, application threads, DLL lifecycle, application JIT code, child processes, and DynamoRIO integration are outside this proof of concept.

## Translation and state

The backend emits `MOVUPS`, packed-single legacy operations such as `ADDPS`, `SUBPS`, `MULPS`, and `DIVPS`, legacy logical operations, and `RET`. A VEX.256 operation is split into two independent XMM operations. A VEX.128 write clears the upper 128 bits of the destination YMM, as required by AVX semantics. `VZEROUPPER` clears the upper halves of all 16 logical YMM registers.

The translator does not advertise AVX through CPUID and does not intercept application processes.

## ARM64 development validation

The development sandbox is Linux ARM64 and cannot execute x86 instructions natively. The test script cross-compiles the harness for x86-64 and runs it with QEMU user-mode/Tiny Code Generator configured to expose SSE4.2 while hiding AVX, AVX2, and XSAVE. The harness checks CPUID before running the cases.

This is a functional validation of the covered subset under x86 emulation without AVX. It is not a performance measurement and is not native validation on the Celeron N5100 or Windows 10.

Required Ubuntu packages are `g++-x86-64-linux-gnu`, `binutils-x86-64-linux-gnu`, and `qemu-user`.

```bash
./tests/run-tests.sh
```

The script writes `build/generated_addps256_sse.bin`, its disassembly, and the harness disassembly. Generated executable memory follows a W^X transition: pages are writable during emission and executable afterward. Windows x64 uses RCX as the first argument; SysV AMD64 uses RDI.

## Validation boundary

The tests compare vector state with a scalar reference for controlled values and cover register aliasing, 128/256-bit widths, high registers, copies, and `VZEROUPPER`. They also verify that unsupported forms are rejected and that generated code contains no AVX mnemonics.

This does **not** establish complete ISA equivalence. MXCSR, NaNs, subnormals, alternate rounding, floating-point exceptions, memory faults, and all instruction forms require dedicated tests. It also does not demonstrate that DynamoRIO can load arbitrary AVX code on a host without AVX. Before exposing any AVX capability to an application, the project must integrate transformation into a DBI/runtime, virtualize CPU detection coherently, instrument all executed code including JIT and child processes, reject unsupported instructions diagnostically, and pass differential and exception tests.

## Researched architecture

DynamoRIO is the provisional candidate for a Windows x64 DBI/code-cache layer, using its x86 IR or the project's decoder. Intel XED may support offline tools and tests but does not provide a runtime. Intel SDE remains an external comparison tool and is not modified or redistributed as project implementation. QEMU TCG remains a translation-architecture reference; its system emulation is not equivalent to a native Windows process shim.
