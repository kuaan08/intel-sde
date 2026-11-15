# Xenia CPU Feature Study

## Conclusion

Advertising AVX through `CPUID` may remove the first startup error, but it does not make Xenia functional. The public Xenia Canary source shows at least three independent gates: a startup CPU check, a second check in the x64 backend, and host JIT code emitted by Xbyak using AVX and, when available, AVX2/FMA and other extensions.

SDE AVX Lab would therefore have to translate Xenia's generated x64 code, in addition to instructions present in the executable and code pages created by the JIT.

## Evidence in the Xenia source

`src/xenia/base/main_init_win.cc` creates `Xbyak::util::Cpu` before initialization and exits when `cpu.has(Xbyak::util::Cpu::tAVX)` is false:

```text
Your CPU does not support AVX, which is required by Xenia.
```

Source: [main_init_win.cc](https://github.com/xenia-canary/xenia-canary/blob/master/src/xenia/base/main_init_win.cc)

`src/xenia/cpu/backend/x64/x64_backend.cc` checks AVX again in `X64Backend::Initialize` and logs the following when the check fails:

```text
This CPU does not support AVX. The emulator will now crash.
```

Source: [x64_backend.cc](https://github.com/xenia-canary/xenia-canary/blob/master/src/xenia/cpu/backend/x64/x64_backend.cc)

The `X64Emitter` constructor in `src/xenia/cpu/backend/x64/x64_emitter.cc` also fails when AVX is unavailable. This is the x64 JIT emitter, so the requirement is not limited to startup code.

Source: [x64_emitter.cc](https://github.com/xenia-canary/xenia-canary/blob/master/src/xenia/cpu/backend/x64/x64_emitter.cc)

`src/xenia/base/platform_amd64.cc` detects additional features and creates emission flags for AVX2, FMA, BMI, MOVBE, GFNI, and AVX-512. The `x64_extension_mask` documents AVX1 as the floor and enables AVX2/FMA when detected.

Source: [platform_amd64.cc](https://github.com/xenia-canary/xenia-canary/blob/master/src/xenia/base/platform_amd64.cc)

## Evidence from the analyzed binary

The analyzed `9eedf60` release contains `CPUID`, `XGETBV`, explicit AVX rejection messages, and thousands of VEX instructions. The inventory includes scalar and packed-single operations, conversions, comparisons, AVX2 integer operations, shuffles, packs, blends, `VEXTRACTF128`, `VINSERTF128`, and memory operations.

The detailed inventory is maintained in the project research records and must be treated as a release-specific static profile, not a complete runtime coverage claim.

## Why changing CPUID is insufficient

If only the AVX bit is advertised, Xenia may pass the first gate while Xbyak still emits AVX instructions for host JIT blocks. On a Celeron N5100 without AVX, one of those instructions can raise `STATUS_ILLEGAL_INSTRUCTION` as soon as the block executes.

Advertising AVX2 or FMA without implementing them can select even more unsupported paths. Correct virtualization must be conditioned on actual backend coverage, including `CPUID`, `XGETBV`, XMM/YMM state, executable pages, threads, exceptions, and JIT code.

## Correct SDE AVX Lab strategy

The first target should not be to trick Xenia. It should be to build a dispatcher that receives executed x64 blocks, identifies VEX/EVEX instructions, translates covered families to SSE/scalar sequences, and maintains guest state per thread.

For Xenia, coverage should begin with `VMOV*`, `VADDSS`, `VSUBSS`, `VMULSS`, `VDIVSS`, conversions, comparisons, and `VZEROUPPER`. Frequently used AVX2 integer families such as `VPADDD`, `VPAND`, `VPOR`, `VPXOR`, shifts, packs, unpack operations, and `VPSHUFB` belong to later milestones. Only after this coverage exists should AVX be exposed to the process and the next JIT path measured.

The official [Xenia Canary Quickstart](https://github.com/xenia-canary/xenia-canary/wiki/Quickstart) lists AVX or AVX2 as a minimum CPU requirement. The [FAQ](https://github.com/xenia-canary/xenia-canary/wiki/FAQ) explains that the no-AVX message means the processor does not meet that requirement.

## Current project state

SDE AVX Lab has an initial slice of `VMOVSS`, `VADDSS`, `VSUBSS`, `VMULSS`, and `VDIVSS`, but it does not yet have the general Windows dispatcher required to intercept Xenia's JIT. AVX therefore remains closed by default.
