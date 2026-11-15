# Public Intel SDE Feature Map and Independent Implementation Plan

This document records a clean-room feature comparison. It is based on public Intel documentation, the authorized Windows 9.58 manual retained in this repository, and observed project requirements. It is not an extraction of Intel SDE internals.

## Publicly documented capabilities

Intel describes SDE as an application-level binary instrumentation emulator built on Pin and XED. Its public product page identifies instruction emulation, CPUID presentation, a basic emulator, dynamic instruction-mix histograms, ASCII execution tracing, footprint analysis, and XED disassembly as separate capabilities. [1]

The Windows 9.58 manual documents a launcher form using emulator options before `--` and the target application after `--`. It also documents CPU profiles, an emulate-everything shell mode, environment-variable forwarding, error-file output, attach-to-process mode, CPUID control, and emulation controls such as forced emulation, FMA policy, XINUSE policy, XSAVE policy, and illegal-instruction reporting. [2]

Intel's release notes show that later SDE versions continued adding instruction families and platform behavior, including AVX/AVX2, AVX-512, AMX, AVX10, APX, CPUID fixes, Windows improvements, JSON mix output, CET-related behavior, and tracing improvements. [3]

## What SDE AVX Lab can implement independently

### High priority

**Stable launcher contract.** Keep the `backend-options -- application arguments` model, per-application profiles, working directory, environment forwarding, error logs, and reproducible diagnostic bundles. These are product behavior, not copied implementation.

**Instruction mix and trace.** Add an independent runtime profiler that records decoded opcode, vector width, source/destination registers, basic-block address, memory operand class, execution count, and translation time. Export JSON and a human-readable report. This is the most useful SDE-like feature for deciding which AVX instructions a game actually uses.

**CPUID/XGETBV policy layer.** Add a guest feature profile that is derived from implemented coverage. The layer must hide AVX until the runtime can execute the advertised subset. It should log every intercepted feature query and reject inconsistent configurations.

**Illegal-instruction and unsupported-form reporting.** Add structured error records containing process, module, address, bytes, decoded fields, feature requirement, and fallback result. The runtime should fail closed rather than silently reinterpret an unsupported instruction.

**Code legality checks.** Add an offline scanner and a runtime checker that identify unsupported VEX, EVEX, AMX, FMA, and AVX2 forms before or during execution. This is safer than claiming broad support based on a CPUID mask.

### Medium priority

**Footprint analysis.** Count unique cache-line-sized memory regions accessed by translated blocks. Keep it independent from instruction semantics and expose it as an optional profiling mode.

**Basic-block and trace reporting.** Record hot blocks, decoded instructions, translation decisions, cache hits, cache misses, and fallback-interpreter counts. A top-N report will guide optimization without requiring proprietary implementation knowledge.

**Environment and child-process policy.** Forward selected environment variables and define whether child processes inherit the runtime. Implement this only after the Windows process layer can safely instrument loader-created threads and child processes.

**Attach mode.** Treat process attach as a later Windows-only feature. It requires suspension/coordination of threads, module enumeration, instruction-cache discovery, and safe handling of code that executed before attachment.

### Defer until the core is correct

**Record/replay, debugger integration, CET, TSX, XSAVE/XINUSE, FP16 accuracy modes, FMA modes, AVX-512, AMX, AVX10, and APX.** These are valuable but depend on a correct process runtime, complete architectural state, exception behavior, and a clearly defined compatibility target. They should not be exposed as supported options in the launcher yet.

## Current project coverage

The repository currently contains an independent AVX1 VEX decoder/interpreter, an AVX1-to-legacy-SSE proof of concept, a Windows launcher, profile storage, diagnostics, external `-mix` integration, and no-AVX CI tests. It does not yet provide process-level DBT, memory operands, a block cache, CPUID/XGETBV interception, attach mode, or an independent dynamic mix/trace engine.

The next implementation should therefore add an internal IR plus a profiler interface before adding more opcode families. Both the interpreter and the SSE lowering backend should report the same decoded instruction identity so that their results can be compared.

## Public feature mapping

| Public capability | Independent status | Recommended project component |
|---|---|---|
| Launch an application under an emulator policy | Partial | Windows process runtime |
| CPU profiles and CPUID presentation | Profile UI only | Guest feature policy |
| AVX/AVX2 instruction emulation | AVX1 subset only | Decoder, IR, interpreter, SSE backend |
| Dynamic instruction mix | External backend only | Runtime profiler and JSON writer |
| Instruction/register/memory tracing | Not implemented | Trace sink and block observer |
| Footprint analysis | Not implemented | Cache-line footprint collector |
| Error file and illegal-instruction diagnostics | Basic launcher log | Structured runtime error report |
| Child-process coverage | Not implemented | Process tree policy |
| Attach to running process | Not implemented | Windows attach engine |
| Record/replay | Not implemented | Deterministic trace subsystem |
| Broad future ISA coverage | Not implemented | Feature-gated milestone modules |

## References

[1]: https://www.intel.com/content/www/us/en/developer/articles/tool/software-development-emulator.html "Intel Software Development Emulator product overview"
[2]: https://github.com/kuaan08/intel-sde/tree/master/windows-9.58/doc "Authorized Intel SDE Windows 9.58 manual retained in this repository"
[3]: https://www.intel.com/content/www/us/en/developer/articles/release-notes/intel-software-development-emulator-release-notes.html "Intel Software Development Emulator release notes"
