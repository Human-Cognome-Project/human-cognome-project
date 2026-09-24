# Engine architecture boundary

This file records the current HCP engine boundary after recovery of the native
C++ workspace that had remained local during the September 2026 repository
reconciliation.

## Runtime stack

1. **Modified Taichi fork** — the portable execution/compiler substrate. HCP
   retains Taichi's mass-particle solving machinery, mathematical execution
   model, and backend compilation/device machinery while removing or widening
   assumptions that were inappropriate for small-data field identifiers.
2. **Native engine support** — `src/engine/`, a thin C++ layer over the fork:
   instance lifetime, kernel compilation/launch, device inspection, and bulk
   host/device transfer.
3. **Field physics** — `src/field/`, the native C++ field/tick implementation
   linking `engine_support`.
4. **Engine harness** — the control structure around the physics engine. It is
   the surface the future analyst will use to compose working state, add/remove
   or adjust elements, run/reset analysis, and inspect engine state.
5. **Analyst functions** — future work. No analyst reasoning implementation
   exists yet.

The production path is C++. Python is not an HCP engine or data-layer
implementation language. Python present inside the Taichi fork is upstream
frontend/build/test material; HCP bypasses that abstraction operationally.
Project Python is permitted only where it provides convenient bootstrap,
migration, import/export, or other offline I/O.

## Naming note: field::Harness

The recovered native field code contains a class named `field::Harness`.
That class owns the field model's device arrays, compiled kernels, staging
state, and tick operations. The name predates the later architectural
distinction above.

Do **not** infer that this class by itself is the complete analyst-facing
engine harness. Its name is retained during recovery to avoid gratuitously
renaming working code. The wider harness includes CPU-resident composition,
working-set/focus control, and the eventual analyst control surface.

## Modified Taichi fork

The fork is expected at `engine/taichi/`.

The recovered modernization record shows at least these HCP-relevant changes:

- configurable SNode/SNode-tree startup capacities replacing fixed metadata
  ceilings;
- paged/lazy ListManager metadata rather than the prior heavyweight fixed
  allocation pattern;
- dense external-array flattened offsets widened to i64 in shared LLVM codegen;
- dense `Ndarray` element-count accumulation widened to 64-bit;
- runtime/AOT ABI handling for the modified runtime;
- CPU and CUDA validation of the modified paths, including a >2^31-element
  dense-array regression test.

This is targeted modernization, not a claim that every Taichi backend or every
logical index in the full upstream system is globally 64-bit.

## Analyst-supporting data kernels

Database/cache work lives under `/kernels/database/`; the WAL manager is its
peer under `/kernels/wal/`. They support the future analyst by keeping
working surfaces current. They are not part of the physics-engine harness.

Shared endpoint/box/scheduler infrastructure lives under
`/network/endpoint/`. Future configuration/topology, serialization bridges,
and thread management belong to the network layer.

## Current confidence boundary

The recovered native engine performs viable calculations and substantial CPU
and CUDA functional validation was recorded. The code and fork are still only
partially vetted as a whole. Performance and scaling under realistic HCP load
remain empirical questions; existing measurements are evidence, not production
performance guarantees.
