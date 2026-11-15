# Backend Strategy

## Primary backend: SDE AVX Lab Runtime

The independent SDE AVX Lab Runtime is the product being actively implemented. It owns the decoder, intermediate representation, memory semantics, translated-block cache, Windows process layer, CPUID/XGETBV policy, thread state, exception handling, AVX1 coverage, and later AVX2 coverage.

The launcher must keep this backend selected by default. Every new performance, compatibility, and correctness feature should be implemented and tested here first.

## Separate fallback: Intel SDE 9.58

Intel SDE remains an optional, explicitly labelled fallback for users who need to run an application before the independent process runtime is complete. It is not part of the SDE AVX Lab implementation, is not modified, and must not receive project code or patches.

The fallback is useful for:

- confirming that an application actually requires a given ISA feature;
- collecting an external instruction mix for prioritization;
- comparing application startup and behavior;
- providing a temporary compatibility path for users.

Fallback results must be recorded separately from native SDE AVX Lab results. A game running under Intel SDE does not prove that the independent backend works.

## Product boundary

The launcher may expose both entries, but they have different contracts:

| Entry | Role | Current behavior |
|---|---|---|
| SDE AVX Lab Runtime | Primary product | Runs the independent core/self-tests; process DBT is under construction |
| Intel SDE 9.58 | Optional fallback | Launches a selected Windows application under the external Intel SDE kit |

The project should not remove the fallback until the independent backend can replace it for the target application set and its results are reproducible.
