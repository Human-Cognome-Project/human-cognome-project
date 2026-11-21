# Bounded active workbench — Stage D

**Current scope (2026-10-23):** This is the retained bounded **v1/FixedStudy**
workbench. #153 completed the type-phase simplification previously requested in
#152. Earlier local-test prerequisites below are historical; the full operational
GPU handoff now follows [#170's integrated CPU exit](../../docs/core-operation-readiness-plan.md).
[#171 structural v2](../../kernels/database/warm/STRUCTURAL-RECIPES.md) is built,
but enduring owner refresh, admission and lifecycle remain under #156. Do not
copy supplied-start/equal-count refresh behavior into that operational path.

**Explicit prepared entry — 2026-11-18 (awaiting review):**
[`ActiveSession::submit_prepared`](ACTIVE-PREPARED-SUBMISSION.md) is a separate
opt-in operation for the supported shared v2/f64 subset. It accepts pinned
source/model/resource choices, provisions its own Program-owned layout/Harness,
delegates to the existing prepared owner, and publishes a handle after bootstrap.
Core and viewport observations use its native completed state. Legacy `submit`,
tick and refresh semantics below remain v1; prepared tick is explicit and legacy
refresh cannot replace a prepared execution. This does not migrate the retained
capture executable or qualify actual retained source acquisition.

The [legacy source-layout lifetime inspection](LEGACY-SUBMISSION-LIFETIMES.md)
at `a0ec440b` observes the real `submit` path: registration discards its returned
tree handle, so late parameter failures and successful Execution teardown leave
source roots live until the pre-Runtime-finalization boundary. Activity roots
retire independently. The proposed existing-Execution ownership correction must
drain and destroy FixedStudy before source retirement. This inspection changes
no production lifetime, legacy semantics or numerical behavior.

The authorized [Execution ownership correction](SESSION-SOURCE-TREE-OWNERSHIP.md)
at `e226b76a` captures the source handle on both submit paths and uses the existing
owner/drain with ordered member destruction. Prepared owner dies before Harness;
legacy FixedStudy dies before native source retirement. Failed candidates retire
before returning, successful bindings survive through following ticks, and
Runtime remains alive throughout teardown. Numerical/public operation semantics
and source-pin boundaries are unchanged.

The [retained-to-C3 source trace](RETAINED-C3-HANDOFF.md) (inspection awaiting
review) distinguishes this legacy session from the built bounded prepared-owner
route. `submit` uses v1 binding/FixedStudy and refuses v2; it does not provision a
prepared Harness or accept explicit comparison/resource bindings. The existing
prepared owner already supplies native bootstrap and actual claim publication.
The remaining retained-entry request/lifetime operation is a separate boundary;
it should delegate to that owner rather than duplicate native installation.

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

## Native implementation split — 2026-11-15

Contributor implementation under #343, awaiting review. Public source
`7b998501d07c85384c827d4768a7cfc120f38035` builds on #342 at
`9b5c19261998a146b9152904fb948913bd509f5f`, which incorporates main
`a2f192cf` and #345's qualified publication correction. #342's structural source
is accepted for bounded continuation but remains a separate pending integration
dependency; this move does not accept or merge it.

[`active_workbench.cpp`](../src/assembly/active_workbench.cpp) now contains the
logical `ActiveAssembly` implementation, including #342's inherited/replaced/
cleared focus and revision/restart distinction.
[`active_session.cpp`](../src/assembly/active_session.cpp) contains
`ActiveSession::Execution` and all session methods. The `active_workbench` target
builds both files with the same dependencies and flags. The public header,
class layouts, function signatures, handle/report rules and teardown order are
unchanged. The internal validation helper is copied with identical behavior;
`<chrono>` moves with its users.

This is an implementation-file boundary. The public header still includes the
native study interface, and logical geometry validation still uses
`enclosing_radius` in study preparation. No runtime-free logical target, new
Analyst API, prepared admission bridge, scheduler or numerical change follows
from the move. In particular, retained session execution still uses FixedStudy;
#342's structural handoff is not an implicit C3 execution migration.

Local qualification at the public source above, actual x64, Clang/LLVM 15
Release:

- Quick local CI passed: three CTests, 632 independent placement-math samples
  and placement capacity, using the matching CPU-only Taichi build.
- Source comparisons prove the complete moved session block is byte-identical
  to the #342 base, logical implementation is identical apart from the relocated
  include, and the validation helper is identical. Public header, tests, field
  code and substrate are unchanged. This is source identity evidence, not a
  claim of identical linked binaries or performance.
- `active_workbench_test` passed its 165 checks, including the inherited
  structural/focus/private-input controls and the existing native lifecycle,
  owner, stale-handle/report and restart checks. `viewport_snapshot_test` passed.
  The two native CTests completed in 21.02 seconds.
- A fresh GCC 11 standalone configure/build passed all four projection,
  membership, structural recipe and bootstrap-reservation CTests in 1.03 seconds,
  without Taichi/PostgreSQL dependencies.

Evidence directories: `/tmp/hcp-session-quick-7b998501` and
`/tmp/hcp-session-focused-7b998501`. Both checksum manifests verified. Archive
`/tmp/hcp-active-session-7b998501.tar.gz`, SHA256
`84844fc0bbfde352c4c0e1b5279f317c5b7462c62d16296f0123fcf0f3bf1879`,
retains the source/base pins, exact comparison/build/test commands, before/after
source blocks, toolchain/configuration and binary pins, complete test logs and
post-PASS offline-cache warnings. This is contributor-local evidence, not
independently inspected hosted evidence. Final follow-up changes documentation
only. Full local regression, DB, browser, GPU, extra compiler modes, pinning and
performance qualification were not run. Maintainer-reviewed local evidence may
permit bounded continuation; authoritative hosted disposition still governs
final integration.

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
