# Documentation

This directory contains the maintained technical documentation for SDE AVX Lab. The project is an independent research runtime; the documentation distinguishes implemented foundations from process-level capabilities that are still under development.

## Start here

| Document | Purpose |
|---|---|
| [Architecture](ARCHITECTURE.md) | Runtime layers, state model, and translation boundaries. |
| [Backend strategy](BACKEND-STRATEGY.md) | Comparison of interpreter, SSE lowering, DBT, and future backends. |
| [Building and testing](BUILDING.md) | Supported build environments, commands, and validation rules. |
| [Next engineering milestones](NEXT-MILESTONES.md) | Acceptance gates and implementation order. |
| [Windows process runtime](WINDOWS-PROCESS-RUNTIME.md) | Current Windows host, executable-page, thread, and exception work. |

## Engineering and compatibility studies

- [Intel SDE feature map](INTEL-SDE-FEATURE-MAP.md) — external behavior reference and project boundaries.
- [Xenia CPU feature study](XENIA-CPU-FEATURE-STUDY-2026-11-15.md) — static analysis of the Xenia CPU/JIT requirements.
- [Legal boundary and optimization plan](LEGAL-BOUNDARY-AND-OPTIMIZATION-PLAN.md) — independent implementation rules and optimization gates.
- [External Windows SDE 9.58 manifest](EXTERNAL-WINDOWS-SDE-9.58-MANIFEST.txt) — provenance and hashes for the preserved comparison package.

## Historical material

The [`legacy/`](legacy/) directory contains project research and feasibility records that informed the current architecture. The files are kept for traceability, but the maintained status is defined by the documents above and by the root [README](../README.md).

- [Feasibility and architecture record](legacy/FEASIBILITY-AND-ARCHITECTURE.md)
- [Research sources catalog](legacy/RESEARCH-SOURCES.md)

## Documentation conventions

- All maintained project documentation is written in English.
- Dates in filenames use `YYYY-MM-DD`.
- “Implemented” means that code and deterministic tests exist; it does not imply complete Windows application compatibility.
- Experimental results identify the environment used. A QEMU or Wine result is not presented as native Windows or reference-hardware validation.
- Third-party package documentation and license files remain in their original locations and are not rewritten.
