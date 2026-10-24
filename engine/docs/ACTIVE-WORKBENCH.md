# Bounded active workbench — Stage D

**Current scope (2026-10-23):** This is the retained bounded **v1/FixedStudy**
workbench. #153 completed the type-phase simplification previously requested in
#152. Earlier local-test prerequisites below are historical; the full operational
GPU handoff now follows [#170's integrated CPU exit](../../docs/core-operation-readiness-plan.md).
[#171 structural v2](../../kernels/database/warm/STRUCTURAL-RECIPES.md) is built,
but enduring owner refresh, admission and lifecycle remain under #156. Do not
copy supplied-start/equal-count refresh behavior into that operational path.

Accepted/merged in [#149](https://github.com/Human-Cognome-Project/human-cognome-project/pull/149)
at `867c310`, under [#148](https://github.com/Human-Cognome-Project/human-cognome-project/issues/148),
after A/B/C and [design #147](../../docs/pre-d-hot-workbench-design.md) acceptance.
This is native C++ integration using supplied requests, not an analyst or scheduler.

**GPU validation status (2026-10-19):** #150 is accepted/merged at `755bcefb`,
including focus-local motion, native activity and combined CPU/PostgreSQL
acceptance on both host paths. The CPU-review prerequisite for bounded local
GPU tests is satisfied; actual hardware/pinning evidence remains under #145.
#152 requests a focused type-phase activity simplification. The command examples
exercise the accepted baseline; they are not evidence of executed GPU tests.

`ActiveAssembly::focus()` supplies exact prepared field keys without manager
access. Focus configuration survives logical revision/restart. Preparation
subtracts a nearby double reference before narrowing to f32; numerical reports
carry local positions and `frame_anchor` separately. Owner checks apply to both
full reports and `ActiveSession::demand()`, including private-derived focal data.
The revised shared checks cover translated unit motion, centroid propagation,
failed focal refresh and controlled restart on BulkCopy and CpuDirect. The
original D evidence below records its preceding baseline, not the combined run.

`warm::SharedManager` wraps a dedicated shared-only Stage A store. Its public
source directory delegates only `00`/`02` prefixes. A non-shared scope is refused
before prefix/source resolution; all recipe keys are gated before storage
preparation. Errors contain no supplied codes/payloads. Only this publication
path mints `SharedTicket`s. A ticket identifies an immutable public publication;
if its head changes, acquisition refuses before adding any lease. Existing owned
pins retain A's epoch behavior and cannot be removed by a failed acquisition.
The embedding owner supplies
only public credentials and keeps the generic `Toolbox` API out of request scope.
This does not turn A's generic private-storage capabilities into manager authority.

Public recipe inputs must be explicitly public. Scope checking cannot establish
that a caller secretly derived shared-key geometry from private information.
Active inputs use a separate type/API with no publication method; they never
become a joint `warm::Bundle` or pass back to the manager.

`assembly::ActiveAssembly` retains the pinned shared recipe, B/C projection and
an optional supplied immutable `LocalInput`, outside any Taichi Program. Local
references carry instance scope, owner and version. The owner attests snapshot
coherence; no private DB is opened and random fixture versions do not claim
real-source verification. The bounded fixture supports supplied root-unit local
primitives, explicit context memberships on named live occurrences, placements
and geometry overrides. It neither infers context nor automatically exposes
hidden component fields. Equal addresses in different scopes stay distinct.
Local groups add no live particle unless a `LocalValue` is explicitly supplied.
All local fixture identifiers are synthetic; no production addresses are allocated.
Native preparation deduplicates identical membership requests; parent ordinals
and the shared source membership graph remain intact.

`inputs_available()` reports presence of required inputs. It does not claim
assembly validation or backend readiness. `bind()` creates a joint native input;
`prepare()` checks coherent geometry, active references, exact mass and explicit
units before backend allocation/compilation. The session then stages bulk scalar
arrays and seeds centroids. Only completion of transfer and seeding makes its
execution handle ready. Failed/incomplete refresh invalidates execution, with
no fallback to manager access. A valid equal-count generation can recover it.
Exposure changes/new layouts need a new study, even at equal counts.

`ActiveSession` owns all runtime bindings and fixed studies. Handles carry
session, study and generation UUIDs; old handles/reports fail validation after
refresh or restart. Reports include exact scoped occurrence/source provenance
and the analyst snapshot, go only to their active owner, and retain no SNode
pointers. They are active values, never manager reports. Call `accept()` when
admitting a retained report into a current session; old report copies are not
current execution state. Owner arguments are trusted embedding capabilities,
not an authentication system against malicious native code.

Session destruction synchronizes, destroys studies/kernels/arrays while their
Program lives, then finalizes it. `ActiveAssembly::restart()` reconstructs from
retained logical inputs with a new study UUID. Shared DB leases belong to the
outer client and remain held across teardown. Missing local inputs keep the
joint study incomplete. This is controlled component restart, not arbitrary
checkpoint recovery or isolation from a process-fatal native/GPU fault.

## Scalar transfer and measurement

The accepted ABI is unchanged: f32 particles `{11,P*}`, groups `{8,G*}`, edge
shares `{1,E*}`; i32 edge indices `{2,E*}` and contacts `{2,K*}`, where `*` means
`max(logical_count,1)`. Row stride follows physical shape, not reserved capacity.
Logical zero-sized determiner reports remain empty. `engine::PackedView` checks
scalar type, positive shape, product/byte overflow, alignment, bounds and a sealed
generation. Views are borrowed only inside synchronous operations. A single
owner must not mutate/resize staging during a call. No async submission or
early-reuse API exists; synchronization precedes return/reuse. These views are
not independent asynchronous leases or a claim that ordinary vectors are pinned.

Default `HostPath::BulkCopy` retains resident Ndarrays on CPU/GPU, with five
bulk payload uploads per successful publication. The bounded `CpuDirect` path
binds the same host vectors as external arrays, allocates no numerical Ndarrays
and makes no payload upload/readback copies. It is refused on CUDA. The seven
field kernels and their static schema/IR are identical; runtime values, scoped
identities, private geometry/context and reports never enter compiler names or
IR constants. Each study construction still invokes compilation/cache lookup;
no zero-compilation-overhead claim is made. Tick now synchronizes so timings and
borrowed-host completion have a defined boundary.

The density experiment fills final packed views directly, with no extra full
repack. Its diagnostic reads all five uploaded spans before seeding and compares
scalar bit patterns, including i32 edges/contact sentinels. Diagnostic readback
is excluded from reported transfer/reporting time and byte counts. Compilation
includes cache lookup; first launch includes lazy materialization. Integration
`assembly_ms` measures binding, layout materialization and preflight; FixedStudy
also performs its existing validation/staging. The density experiment separately
measures packing, allocation, compilation/cache, upload, seeding, first tick,
steady ticks and reporting. Each run reports P/N/G/E/K, payload bytes and process
minor/major page faults. Kernel timing includes synchronization/launch overhead.

The fixture matrix is P/G/E = 16/4096/65536, 4096/32/8192,
4096/4096/65536 uniform, and the same counts skewed onto 16 participants.
All P slots carry mass; degree-zero live slots in the skew case remain live.
Each case has K=0, one full upload and four dt=0 ticks. This isolates layout,
transfer and reduction shape; it does not measure contact/update-frequency
variation, production rollups or production CPU/GPU crossover.

Measured small-fixture centroid/force budgets are absolute `1e-5`/`1e-6`.
Density reductions use centroid absolute `1e-3` and force absolute `0.005` plus
relative `0.001` to allow high-degree f32 accumulation. Mass/identities/ordinals,
counts and initial scalar transfer stay exact. A separate integrated 16-step
nonzero-dt fixture checks finite bounded state and unchanged live mass. No
cross-exposure, backend or restart trajectory equality is asserted.

Unmeasured: host registration/pinning, device bandwidth, async completion,
backend argument-buffer allocator counts, true compiler cache-hit attribution,
and balanced cold/warm performance trials. Copy runs precede direct runs and
may populate caches; their raw timings cannot establish a speedup. Numeric
payload copy counts follow the five bulk calls (zero for direct); runtime/compiler
internal copies and allocations are not counted. These limits and actual GPU
qualification remain in [#145](https://github.com/Human-Cognome-Project/human-cognome-project/issues/145).

## Reproduce

Build the matched native Taichi fork as described in [engine setup](../README.md),
then configure the optional libpq client (PostgreSQL headers/library >=16):

```sh
cmake -S engine -B engine/build -DHCP_WITH_WARM_INTEGRATION=ON
cmake --build engine/build -j 2
ctest --test-dir engine/build --output-on-failure
. engine/build/engine-env.sh
engine/build/active_transfer_experiment
bash engine/scripts/run_warm_active_acceptance.sh engine/build \
  YOUR_OWNED_CONTAINER YOUR_RETAINED_CORE_DB YOUR_RETAINED_LINGUISTICS_DB
```

The container helper packages the built native executable, matching Linux
loader/libraries and runtime bitcode. It creates only a new warm fixture DB,
retains it/evidence, and compares complete before/after cold data-only hashes
(excluding only pg_dump restriction nonces), including on failure. No retained
store is reset, dropped or truncated. The client uses fresh connections, owned
pins and a coherent paired epoch/policy rotation. The hosted CPU job restores
verified paired exports only into new cold fixtures and runs the same client.

After #91's required math/CPU validation, for Patrick's GPU run build against
a CUDA-enabled matched fork and use the
existing connection configuration to supply `HCP_WARM_TEST_CONNINFO` naming a
**new empty warm DB with schema.sql installed**, and the two retained read-only
source conninfos `HCP_CORE_TEST_CONNINFO`/`HCP_LING_TEST_CONNINFO`:

```sh
. engine/build/engine-env.sh
engine/build/warm_active_integration --cuda
engine/build/active_workbench_test --cuda
engine/build/active_transfer_experiment --cuda
```

The GPU commands run the same fixtures with resident arrays, including restart;
they do not enable direct external host binding or explicit pinning. GPU work
is unexecuted in this CPU-only cloud environment. See the adjacent
[acceptance record](../tests/active_workbench/acceptance.json) for executed evidence.
