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
   `src/assembly/` now provides the bounded fixed-study preparation/publication
   part, as a separate `study_assembly` target linking `field_core`. It does
   not supply general recomposition or the future analyst control surface.
5. **Analyst functions** — conceptually designed within the overall architecture.
   Concrete implementation structure, engineering and calibration are deferred
   until they can be grounded in engine results on real data structures; this
   does not defer the overall analyst architecture. See the clarification below.

**Pre-D clarification (Patrick, 2026-10-16):** warm is the shared tool/part box;
hot is the analyst-owned host workbench where objects and relationships are
assembled; GPU memory is where prepared numerical objects run together. Private/
local inputs enter only at active hot assembly, through a separate analyst-owned
boundary. The cache manager must not access private DB codes/data or route them
through warm publication/refresh/reporting; reject private scopes before source
lookup, without content inspection. See the accepted [pre-D design](../docs/pre-d-hot-workbench-design.md).
It investigates native compilation/value boundaries, portable host staging,
field density and restartable Taichi within the continuously operating system.
Issue #148 released bounded D after design #147 acceptance. The
[active workbench adapter](docs/ACTIVE-WORKBENCH.md) implements supplied local
inputs, checked native sessions and controlled restart. GPU pinning, broader
rollups and process-failure isolation remain separate work.
Stage D is accepted/merged in #149. The revised #150 integrates focus-local
coordinates, activity/focal control on both host paths and combined CPU/SQL
evidence; #150 is accepted. The focused #152 revision removes duplicate particle
sets: one activity set per type, native phase traversal, stored motion state
inside active work and ordered contact effects. GPU execution remains local. Dynamic focus-scoped LoD
selection and transition rules remain intended work, as distinguished in the
[field model](docs/ACTIVE-FIELD-MODEL.md#focus-scoped-lod-intended-versus-built).

The production path is C++. Python is not an HCP engine or data-layer
implementation language. Python present inside the Taichi fork is upstream
frontend/build/test material; HCP bypasses that abstraction operationally.
Project Python is authorized only for bulk database population during
bootstrap, including preparation and validation of those batches. Offline
operation alone does not authorize Python components.

## Analyst design and engineering status

Patrick clarified on **2026-11-07** that analyst functions are conceptually
designed. They are part of the overall HCP architecture, not an unspecified
layer to invent after the engine is finished. Their concrete implementation
structure must be engineered and calibrated using engine results on real data
structures. This dependency defers implementation structure and calibration,
not the overall architecture or conceptual functions.

Maintain the distinction between conceptual design, engineered implementation
and empirical qualification. Existing instructions against speculative analyst
behaviour prohibit substituting invented behaviour or prematurely implementing
uncalibrated mechanisms; they do not mean the conceptual architecture is absent.
The engine PoC supplies evidence for that engineering rather than claiming
implemented analyst reasoning.

## Naming note: field::Harness

The recovered native field code contains a class named `field::Harness`.
That class owns the field model's device arrays, compiled kernels, staging
state, and tick operations. The name predates the later architectural
distinction above.

Do **not** infer that this class by itself is the complete analyst-facing
engine harness. Its name is retained during recovery to avoid gratuitously
renaming working code. The wider harness includes CPU-resident composition,
working-set/focus control, and the eventual analyst control surface.

### Field implementation factoring (contribution awaiting review)

The first slice of [#229](https://github.com/Human-Cognome-Project/human-cognome-project/issues/229)
moves `Harness::build_numerical_kernels` into `src/field/numerical_kernels.cpp`,
compiled by the same `field_core` target. The private method declaration and its
two callers (Harness setup and TickExecution setup) remain unchanged. The builder
still returns the same seven kernels, in the same order, with the same expressions,
names, flags, strict-math choices and native selection inputs. No class member,
allocation boundary, ownership transfer, publication or cleanup order is changed.

Builder-only IR helpers move with the method. Three small file-local helpers
(`store`, `accumulate`, `array_param`) are also needed by prepared inspection in
`field.cpp`; both compilation units retain identical private definitions. No new
header or widened access is introduced. The remaining `field.cpp` owns construction,
transfer/inspection, launches and execution coordination as before. This first move
does not claim that those responsibilities are already separated.

Complete owner peak/lifetime/work accounting remains the next implementation task.
Its ownership/borrowing and construction-failure/destruction inventory should guide
later cohesive extractions. Keep one readiness/generation/publication authority;
further responsibility or ownership changes require separate review. #229 remains
open beyond this first mechanical slice. Source-pinned CPU regression and compiler
evidence belongs in [local CI evidence](tests/local_ci/README.md).

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
