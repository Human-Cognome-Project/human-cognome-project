# Effective-motion CPU evidence

Status: #150 accepted/merged; #152 type-phase revision for review, 2026-10-19, under
[#91](https://github.com/Human-Cognome-Project/human-cognome-project/issues/91).
The [active field model](ACTIVE-FIELD-MODEL.md#current-selective-tick-and-the-effective-motion-predicate)
specifies the exact calculation. This evidence uses the native C++ path and
the pinned Taichi fork on CPU. Retained cold sources are read-only; tests create
separate warm fixtures and use supplied local values. No private store is opened.
The revision uses Taichi native binary SNode activation/iteration for particle
and centroid scheduling. The focused #152 revision has one set per type and
uses native cell operations directly; stored motion state is read only inside
selected work. See the [phase and storage semantics](ACTIVE-FIELD-MODEL.md#native-binary-activity).

## Calculation under review

Candidates combine every connected field and applicable contact contribution,
then apply carried velocity, whole-mass integration, the existing brake and
the focal radial term. A wake requires a final **local** stored-f32 positional change.
A fixed full position/velocity state can sleep until a relevant input changes;
zero speed alone does not exclude it. A changed centroid can trigger a
candidate calculation without waking that particle.

Participating-mass movement ratios flag centroids before placement, with
any-active accumulation across members. Retained weighted displacement and
rounding residue preserve collective effects within/across ticks. Placement
runs once per flagged centroid using all members, including sleepers. Group
and universal masses are reused until publication. Stable adjacency ordering
and single-writer reductions make unchanged-input reuse independent of float
atomic scheduling order within a backend.
Field/activity kernels disable fast-math reassociation so the compiler cannot
erase a rounding remainder algebraically. Centroid placement and remainder
arithmetic widen to f64 before publishing f32, avoiding an extra rounded
origin addition. Exact empty-mass guarding replaces the old additive `1e-20`
centroid denominator; a finite-small-mass fixture checks the ordinary mean.

Supplied placements are translated around a participating focal member in
double before narrowing to f32. The focal fields' ordinary weighted centroid
is then calculated locally and used in radial integration. Positions, movement
ratios and centroid propagation stay local. Reports retain the fixed double
reference separately; adding it back as absolute f32 state never controls
inactivity. See the [coordinate calculation](ACTIVE-FIELD-MODEL.md#supplied-focal-fields-and-mathematical-origin).
Automatic focus/granularity, transition rules and analyst behavior remain
intended follow-up rather than implemented policy.

## Executed fixtures

`activity_test` runs with one CPU worker; `activity_parallel_test` runs the same
assertions with two. `activity_direct_test` uses CpuDirect with two workers.
These compare selective results with `AllLoaded`, which
uses the same laws/order but bypasses exclusions. Each comparison checks every
particle scalar for finiteness and absolute difference at most `1e-5`, and
group mass exactly; centroid positions use the same absolute tolerance.
The five-tick ripple uses `1e-4`; translated-focus relative positions use
`1e-4`. Direct expected-value assertions complement this shared-law reference.
This is bounded equivalence evidence, not a global trajectory error bound.

| Fixture | Checked observation |
|---|---|
| Fixed point | No positional wakes; subsequent ticks omit force/integration/placement. Parameter and publication/mass changes invalidate reuse. |
| Ineffective centroid touch | Exact zero-gravity responses are rejected before next-particle activation; carried motion continues while stationary members remain excluded. |
| All connected fields | One changed field wakes a particle that evaluates both its changed and unchanged connected fields. |
| Any-active placement | Two qualifying members trigger one placement, including a sleeping member, even when resulting centroid motion cancels. |
| Collective motion | Individually insufficient field effects combine before particle gating; small member motions combine before centroid gating. |
| Retained residue | Sub-ULP centroid contributions combine over ticks; initial seeding residue also combines with subsequent motion. |
| Carried state | Velocity can accumulate before displacement; nonzero stored speed at `dt=0` can be a fixed state and resumes after `dt` invalidation. |
| Focal origin | Single/multiple weighted focal fields, translated stationary focus, actual radial integration, missing/duplicate/mass-zero rejection and explicit origin reset. |
| Large translated study | Independent unit velocity/dt expectations at offsets 0 and `2^27`: positions 1–4, continued eligibility, half-unit centroid progression, rejected stationary touches and identical demand counts. |
| Bounded ripple | Successive published dependencies reach four linked particles over four ticks, then stop at a participant without displacement. |
| Ordinary contact | Supplied contact adjacency preserves equal/opposite whole-particle response across selective/reference ticks. |
| Contact phase boundary | Seventy live particles, both source index directions and pointer blocks: peers calculate at the following particle phase, first displacement occurs on the expected step, and an unmoved peer does not propagate activity. |

`activity_assembly_test` additionally prepares repeated English letter objects
through the native SNode/membership path, binds a focal field by exact kind/value,
and checks actual radial integration on BulkCopy and CpuDirect. Supplied initial
unit separation and subsequent unit motion survive `2^27` translation before
f32 conversion. Wrong-kind and duplicate focus fail
preparation; invalid refresh disables ticks and a valid refresh recovers.

The accepted #150 baseline remains in its original
[manifest](../tests/activity/acceptance.json) and CSV files. The focused #152
revision has [separate evidence](../tests/type_phase/acceptance.json), retaining
translated motion, reference comparisons and combined workbench/PostgreSQL
checks. Its native contract includes a controlled CPU live-bit probe: a peer
activated inside traversal can enter that same loop. Production contact writes
therefore occur after the consuming particle calculation pass. Bounded contact
fixtures check both pointer/index directions, first response time and no cascade
through stationary peers. Numerical forces use the same original phase/order.
Tick times include native metadata, launches and compilation warmup; these
fixtures establish behavior and work counts, not a speedup.

## Demand definitions

`Harness::demand()` and `FixedStudy::demand()` return the last successful tick.
Publication/invalidation clears old demand. Counters are resident i64 values;
each tick synchronizes and reads 18 counters plus eight f32 focal-control values
(176 bytes). Full numerical arrays transfer only on explicit `download()`.

| Output | Meaning |
|---|---|
| `particles_loaded`, `particles_exposed` | Allocated logical particle capacity and positive-mass live slots, respectively. Free slots are not reported as skipped live work. |
| `fields_loaded`, `fields_exposed` | Logical group capacity and positive participating-mass groups. |
| `edges_loaded`, `edges_exposed` | Prepared participation edges. Validation requires every edge to reference a live particle, so these counts coincide. |
| `particles_evaluated`, `particles_skipped` | Candidate integrations and live particles omitted this tick. |
| `particles_moved`, `particles_woken` | Actual stored-position changes; of those, transitions from no movement on the preceding tick. |
| `force_requests` | Candidate-connected field edges, including exact zero-response eligibility cases. |
| `force_evaluations`, `force_zero_response`, `force_skipped` | Numerical field expressions after exact gravity/mass eligibility, requests rejected by that exact rule, and edges omitted due to activity, respectively. Coincidence can still produce zero force inside a numerical evaluation. |
| `contact_evaluations` | Contact endpoint calculations; both active endpoints cost two calculations for one pair. |
| `gate_checks` | Candidate positional decisions + changed-centroid relative-vector checks + requested field eligibility checks. |
| `gate_rejected`, `geometry_rejected` | Candidate integrations with no positional change; changed-field relative vectors that compare identical. |
| `touch_adjacency_visits` | Actual moved-particle group/contact incidences and changed-centroid member visits. |
| `force_mask_visits` | Zero; force uses native active traversal, with no loaded-particle mask loop. |
| `centroid_gate_visits` | Membership edges inspected inside native touched-group flagging. |
| `centroid_mask_visits` | Zero; unqualified group bits are consumed after the numerical gate, so placement/publication use native traversal. Native metadata checks are not instrumented. |
| `centroid_touch_edges`, `centroid_reduction_edges` | Edges with actually moved members; all member edges reduced for flagged groups. |
| `centroids_flagged`, `centroids_recomputed` | Unique group flags and placements, not the number of triggering interactions. |
| `native_particle_entries`, `native_centroid_entries` | Active integration and centroid-gate body entries, respectively; each set deduplicates touches. |
| `zero_response_touches` | Changed-centroid member responses rejected before activation by exact gravity/mass eligibility. |
| `invalidation_particle_visits` | Logical slots inspected for whole-study invalidation; zero on a stable selective tick. |
| `focal_origin`, `tick_ms` | Origin used by this tick; elapsed time including launch, synchronization and demand readback. |
| `frame_anchor` | Fixed double supplied-coordinate reference for the local positions/origin. Host metadata, not an extra per-tick device readback. |

Traversal/adjacency counts describe their named bodies, not every instruction.
Native list generation and bitmask metadata costs remain uninstrumented. A
settled tick has zero particle/group calculation entries and zero adjacency
visits; measured tick time still includes kernel launches, native metadata,
synchronization and reporting. Touched groups inspect their own memberships,
including sleeping contributors. Publication and whole-study invalidation can
visit all slots. No logarithmic complexity or production speed claim follows.
The native contract test separately verifies duplicate touches and phase-safe
consumption; it does not establish hardware performance.

## Reproduction and GPU boundary

Build using the matching fork/toolchain as described in the engine setup, then:

```sh
cmake --build engine/build -j 3
ctest --test-dir engine/build --output-on-failure
```

#150's accepted CPU review permits Patrick's bounded local validation under
#145. The #152 single-set revision is a focused follow-up for review; neither
baseline acceptance nor CPU execution asserts GPU qualification. Local cases are:

```sh
. engine/build/engine-env.sh
engine/build/native_activity_test --cuda
engine/build/activity_test --cuda
engine/build/activity_assembly_test --cuda
```

These CUDA commands have **not** been executed in this cloud environment.
Stage D [#149](https://github.com/Human-Cognome-Project/human-cognome-project/pull/149)
is accepted/merged at `867c310` and integrated here. Shared workbench checks run
on both host paths, including a supplied private focal primitive, separate
analyst-owned placements/context, owner-only reports/demand, failed focal
refresh, reconstruction and stale handle/report rejection. The same cases run
through fresh-client PostgreSQL recovery; warm-manager source boundaries remain
unchanged. Focus configuration is logical active state retained across restart,
not a new warm record or a cache-manager selection policy. Each run creates
new warm fixtures and verifies complete retained cold-data hashes unchanged.
Actual GPU/pinning evidence remains under #145. The combined-destination use
of `|F_net|` for brake reach is unchanged and remains a separate open formula.
