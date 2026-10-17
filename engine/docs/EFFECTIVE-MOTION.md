# Effective-motion CPU evidence

Status: implemented for review, 2026-10-17, under
[#91](https://github.com/Human-Cognome-Project/human-cognome-project/issues/91).
The [active field model](ACTIVE-FIELD-MODEL.md#selective-tick-and-the-effective-motion-predicate)
specifies the exact calculation. This evidence uses the native C++ path and
the pinned Taichi fork on CPU. No database population, analyst, private-store
access or model retuning is involved.

## Calculation under review

Candidates combine every connected field and applicable contact contribution,
then apply carried velocity, whole-mass integration, the existing brake and
the focal radial term. A wake requires a final stored-f32 positional change.
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

The supplied focal fields form an ordinary weighted centroid. Its origin is
used in the radial direction/reach calculation. An empty selection explicitly
uses the legacy coordinate origin. The tests check a translated stationary
focus and actual integration, not just a display offset. Automatic relational
focus/granularity and a moving-frame velocity transformation are outside this
bounded implementation. The use of stored-f32 positional resolution is an
explicit proposal for review, not a settled semantic-resolution law.

## Executed fixtures

`activity_test` runs with one CPU worker; `activity_parallel_test` runs the same
assertions with two. Both compare selective results with `AllLoaded`, which
uses the same laws/order but bypasses exclusions. Each comparison checks every
particle scalar for finiteness and absolute difference at most `1e-5`, and
group mass exactly; centroid positions use the same absolute tolerance.
The five-tick ripple uses `1e-4`; translated-focus relative positions use
`1e-4`. Direct expected-value assertions complement this shared-law reference.
This is bounded equivalence evidence, not a global trajectory error bound.

| Fixture | Checked observation |
|---|---|
| Fixed point | No positional wakes; subsequent ticks omit force/integration/placement. Parameter and publication/mass changes invalidate reuse. |
| Ineffective centroid touch | A changed centroid with identical computed relative vector is rejected early; another candidate with zero final displacement does not wake. |
| All connected fields | One changed field wakes a particle that evaluates both its changed and unchanged connected fields. |
| Any-active placement | Two qualifying members trigger one placement, including a sleeping member, even when resulting centroid motion cancels. |
| Collective motion | Individually insufficient field effects combine before particle gating; small member motions combine before centroid gating. |
| Retained residue | Sub-ULP centroid contributions combine over ticks; initial seeding residue also combines with subsequent motion. |
| Carried state | Velocity can accumulate before displacement; nonzero stored speed at `dt=0` can be a fixed state and resumes after `dt` invalidation. |
| Focal origin | Single/multiple weighted focal fields, translated stationary focus, actual radial integration, missing/duplicate/mass-zero rejection and explicit origin reset. |
| Bounded ripple | Successive published dependencies reach four linked particles over four ticks, then stop at a participant without displacement. |
| Ordinary contact | Supplied contact adjacency preserves equal/opposite whole-particle response across selective/reference ticks. |

`activity_assembly_test` additionally prepares repeated English letter objects
through the native SNode/membership path, binds a focal field by exact kind/value,
and checks actual radial integration. Wrong-kind and duplicate focus fail
preparation; invalid refresh disables ticks and a valid refresh recovers.

The complete CPU run and bounded demand output are recorded in
[acceptance.json](../tests/activity/acceptance.json) and its adjacent CSV files.
All 12 suites passed: 86 activity checks per worker configuration and eight
focal letter-assembly checks, alongside the existing regression suites.
The CSV includes measured tick times. Initial ticks can include lazy kernel
compilation; these small cases establish work-count behavior, not a speedup.

## Demand definitions

`Harness::demand()` and `FixedStudy::demand()` return the last successful tick.
Publication/invalidation clears old demand. Counters are resident i64 values;
each tick synchronizes and reads 14 counters plus eight f32 focal-control values
(144 bytes). Full numerical arrays transfer only on explicit `download()`.

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
| `touch_adjacency_visits` | `E + 2K` field/contact endpoint dependency checks. |
| `force_mask_visits` | `P` force candidate-mask checks. |
| `centroid_gate_visits` | `E` member-motion checks in centroid flagging. |
| `centroid_mask_visits` | `3G` group checks in flagging, placement reduction and publication. |
| `centroid_touch_edges`, `centroid_reduction_edges` | Edges with actually moved members; all member edges reduced for flagged groups. |
| `centroids_flagged`, `centroids_recomputed` | Unique group flags and placements, not the number of triggering interactions. |
| `focal_origin`, `tick_ms` | Origin used by this tick; elapsed time including launch, synchronization and demand readback. |

Mask/adjacency counts describe their named phases, not every instruction or
all bookkeeping passes. Begin, force clear, contact selection, integration and
determiner still scan particle masks/state; focus/group bookkeeping also scans
groups. Current touch and centroid-gate scans remain linear in loaded edges.
No logarithmic complexity or production performance claim follows from avoided
force or placement calculations.

## Reproduction and GPU boundary

Build using the matching fork/toolchain as described in the engine setup, then:

```sh
cmake --build engine/build -j 3
ctest --test-dir engine/build --output-on-failure
```

After this math proposal is reviewed and integrated with the accepted workbench
code, the new cases can be included in Patrick's local GPU run:

```sh
. engine/build/engine-env.sh
engine/build/activity_test --cuda
engine/build/activity_assembly_test --cuda
```

These CUDA commands have **not** been executed in this cloud environment.
Stage D [#149](https://github.com/Human-Cognome-Project/human-cognome-project/pull/149)
remains a separate open PR. This branch starts from main after #147, so it
does not exercise D's workbench binding or its optional CPU direct-host path.
Normal integration must adapt that path to the new activity buffers/focal
configuration and rerun D's acceptance tests before the combined handoff.
Actual GPU/pinning evidence remains under #145. The combined-destination use
of `|F_net|` for brake reach is unchanged and remains a separate open formula.
