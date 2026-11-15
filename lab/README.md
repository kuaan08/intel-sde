# External Package Audit Lab

The `lab/` directory contains non-invasive audit tools for the external Intel SDE packages preserved in this repository.

## Preserved packages

- `windows-9.58/` — official Windows SDE 9.58 package used as an external comparison reference.
- `linux-9.14/` — historical Linux SDE 9.14 package retained for reference; it is not the project's primary backend.

The binaries, libraries, Pin sources, signature files, manifests, and Intel licenses remain unmodified. Files under `lab/` are original SDE AVX Lab additions and do not change, extend, or replace the package licenses.

## Audit commands

Audit the default Windows package:

```bash
./lab/sde-kit-audit.sh | tee lab/sde-kit-audit.log
```

Audit the preserved Linux reference:

```bash
KIT_ROOT="$PWD/linux-9.14" ./lab/sde-kit-audit.sh
```

On the ARM64 development sandbox, the audit reads tool metadata through QEMU and does not execute Windows `.exe` files. On native Windows x64, repeat the version check with:

```bat
windows-9.58\sde.exe -version
```

## Windows package reference

The Windows package contains `sde.exe`, `xed.exe`, `intel64/pin.exe`, `intel64/sde-mix-mt.dll`, support DLLs, HTML documentation, CPUID profiles, and the `ia32` and `intel64` trees. The identified binary version is **9.58.0 external**. Profiles include `snb`, `ivb`, `hsw`, `skl`, `glm`, `tnt`, `adl`, `tgl`, `spr`, and `future`.

Intel SDE provides chip selection, emulated CPUID, `-mix`, AVX/SSE detection, and analysis or precision/throughput controls for selected instruction families. These are Intel SDE features, not an AVX-to-SSE backend of SDE AVX Lab. The independent runtime remains necessary to generate host-compatible code.

## Reference Windows command

On the Windows 10 x64 laboratory machine, extract the official package without changing its structure and use the 64-bit variant for x64 applications:

```bat
windows-9.58\sde.exe -version
windows-9.58\sde.exe -snb -- "C:\path\to\program.exe"
```

The `-snb` profile is the first Xenia comparison profile because it matches the recorded handoff experiment. Record startup, menus, loaded content, frame rate, errors, and logs separately. Opening a process window is not evidence of gameplay compatibility.

Keep `windows-9.58` intact, do not mix DLLs from another version, follow the package documentation for the required Visual C++ runtime, and preserve generated logs. The Windows 9.58 package is the relevant external reference for the Windows 10 experiment; the Linux 9.14 package is not interchangeable with it.

## No automatic AVX-to-SSE optimization claim

Intel SDE does not provide a general switch that converts every AVX instruction into native SSE for game FPS. It provides platform/bitness builds, emulated CPU profiles, emulation controls, and analysis tools. For the reference computer, use the Windows `intel64` package and the minimum required CPU profile, starting with `-snb`. Enable FMA, AVX2, or newer profiles only when the target requires them and after measuring the effect.

## Reverse-engineering boundary

This project does not reverse-engineer, decompile, disassemble, patch, or copy implementation code from Intel binaries. The useful and permitted work is documentation review, hashes, metadata, CPUID profiles, authorized external tests, and development of an independent translator. Intel SDE remains an experimental reference/oracle, never modified proprietary implementation.

## Suggested next audit

On native Windows x64, record `sde.exe -version`, `-help`, and `-help-long`, then repeat the handoff `-snb` test. Use `-mix` and AVX/SSE reports to identify the target's instruction families. Build any observation client separately and compare its results with the independent AVX1-to-SSE proof of concept.
