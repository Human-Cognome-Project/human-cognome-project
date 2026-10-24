# Live numerical state and conversion audit

Audit date: **2026-10-24**. Source baseline:
`925d8c7ac8175ca6afe399c631fd35e710d4cfcc`, after #174/#175/#176.
The source-pinned [precision evidence](../tests/precision) also hashes the audited
files. The new [native primitives](NATIVE-PRECISION.md) change only packed double
support and add experimental numerical helpers; the live surfaces below remain
at their audited behavior. This is a conversion inventory, not implementation
of a precision-controlled field path or #177's cascade.

The later [candidate-only upward operation](CENTROID-CASCADE.md), accepted in
#182 under #180, reads the resident body/participation surfaces below without
replacing their live arithmetic, history or physical publication.

The accepted [#183 weighted accounting continuation](../../docs/weighted-centroid-accounting-plan.md)
also identifies exact assembly counts retained before the prepared field boundary
but absent from its committed relationship metadata. Carrying those counts is
planned work, not an implemented widening of the audited share/physical state.

## State owners and publication sites

| State | Current owner/width and writers | Consumers and conversion obligation |
|---|---|---|
| Body position (3), velocity (3), force (3), mass, radius | Harness host staging/readback and backend `particle_array_`: 11 f32 rows. [Field kernels](../src/field/field.cpp) force/contact accumulate force; integration updates velocity and position; prepared commit initializes/clears only new/retired rows. | Force/contact, integration, centroid, focus/report and updater read these rows. Promote operands before products/sums. Source f32 values widened later cannot regain lost detail. Keep physical state distinct from the discarded trial-to-published positional difference. |
| Parameters | [Parameters](../src/field/field.h) gravity, dt, contact stiffness and reach strength are host float; launch scalars are f32. Calibration constants include f32 literals. | Widening a prepared scalar buffer alone cannot recover bits already rounded in the host parameter. A selected wider calculation needs deliberate typed parameter/constants and error/calibration review, while preserving formulas. |
| Determined speed | `determined_`/backend determiner: f32 per particle, written by determine. | Motion reporting/control reads the norm of physical velocity. It is not positional rounding residue; changing its precision/meaning needs a separate declared boundary. |
| Participation share | `edge_float_data`/native edge array: f32. Membership mass product is body mass times share. | Products and ordered reductions in force, centroid and repair must use the selected precision. Exact integer source counts and occurrence identities remain separate. |
| Group published centre and mass | `group_array_`: kCenXYZ, kCenM f32. Seed, centroid finalize and prepared repair/commit publish them. | Force and activity/focal calculations consume published values, including sleeping contributors. Reconcile quantization, old/new centre comparison and mass once per comparison level. |
| Group reduction scratch | kAccXYZ and kAccM in the same eight-row f32 group array. Centroid kernel accumulates ordered relative weighted offsets, anchored to a member on seed; inclusive mass is accumulated on seed. | [Centroid finalizer](../src/field/centroid_math.h) widens before final divide/add, but its inputs are already f32. The helper then narrows centre and weighted remainder to f32. Merely widening this final addition does not widen the reduction. |
| Accepted body displacement | Activity `particle_delta`, three f32 rows, written by integration as stored next minus stored old position. Clear/reset/retirement also write it. | Activity mark uses mass*share*accepted displacement for centroid-change tests. This is physical accepted motion, not an accumulated subthreshold trial remainder. For a quantized profile derive it from accepted published positions at the declared calculation precision. |
| Mixed group pending state | Activity `group_delta[0..2]`: f32 weighted pending displacement. Activity mark adds member displacement; field finalize replaces it with `centroid_coordinate(...).residue`; repair writes a fresh remainder. | It combines physical accounting with positional rounding residue. Mark's combined-effect predicate reads it. Clearing it wholesale can lose physical contributions; preserving it wholesale can reintroduce noise. Exact accounting must be reconciled before live conversion. |
| Previous published group centre | Activity `group_delta[3..5]`: f32, written by finalize and prepared repair. | Touch compares previous/current relative effects on members. This is history for native next-phase wake behavior, not rounding noise. Preserve its coherent relation to the newly published centre. |
| Focal controls | Activity `focus`: 11 f32. Begin writes combined selected-field relative sums, mass, changed flag and origin; commit writes repaired controls; `fixed_origin_` is f32. | Begin reads selected published centres in order around the previous focal reference. Integration/radial damping and wake logic consume it. Precision changes must cover products, sums, division and comparison, not just the final three coordinates. |
| Frame anchor | Harness FrameAnchor: three host doubles; reporting/world-position interpretation adds local f32 values to this anchor. | A double anchor does not make local arithmetic f64. A native-exact translation need not preserve quantization spacing/grid. D6 needs explicit representability and quantization/frame acceptance. |
| Universal mass | One f32 native value/host vector, computed over participating/present bodies by the established setup/update path. | Radial damping/global invalidation use it. Preserve physical mass; this is not a positional remainder. |
| Flags, masks, indexes, handles | Activity flags/CSR are i32; counters are i64; native presence/awake SNodes and map handles retain their existing types. | These do not become f64. Use the existing native type-phase awake selection; no scheduling epochs, second awake registry or loaded-array scheduler is needed for a numerical conversion. |

The exact field layouts are in [field.h](../src/field/field.h), Activity allocation
and phases in [activity.cpp](../src/field/activity.cpp), and launch/readback sites
in [field.cpp](../src/field/field.cpp). Body Moving/PriorMoving, group
Flagged/Changed and the `continued || moved` predicate govern the built profile.
An unchanged quantized coordinate does not authorize zeroing velocity or simply
renaming those predicates. #172/#177 must decide coherent response/activation
before integration.

## Repair, candidates, observations and source narrowing

- [Prepared candidate reduction](../src/field/prepared_update.cpp),
  `prepare_update_execution`: prospective ordered members are reduced in f32
  about the old centre for existing fields and first member for new fields.
  It uses the same finalizer as ticks. Fresh repair already incorporates physical
  member movement and must not add the old pending state again.
- Candidate `derived_array` has `14*max(G,1)+12` f32 elements: eight group rows,
  three new weighted remainders, three old-centre/history rows, universal mass
  and eleven focal controls. Candidate/commit kernel parameters, status/readback,
  and source bindings must all agree on a future wider profile.
- New-row fixture staging has twelve f32 rows per B (eleven body fields and
  initial speed). It is private test input, not production initialization. The
  first precision slice does not turn this into a live numerical input API.
- [Geometry preflight](../src/field/prepared_geometry.cpp) reads f32 live body,
  group and focal arrays. Shared [contact geometry](../src/field/contact_math.h)
  uses f32 constants, squared distances, reciprocal square root, radii and
  penetration; current-focal sums are f32. The query's local focal values/mass
  are f32, its frame anchor double and its counters i64. Candidate pair IDs/CSR
  stay i32. Future conversion must also reconcile contact/occupancy predicates.
- Full prepared inspection stores `3P+6G` f32 motion values plus eleven f32 focal
  controls, f32 body/group/determiner readback and integer state/masks/counts.
  Demand exposes a three-float focal origin, double frame anchor/timing and
  integer work counters. A double report wrapper cannot recover precision lost
  at these boundaries. Reports remain observations, not another live authority.
- [Study preparation](../src/assembly/study.cpp) `numerical(double)` explicitly
  narrows to float after finite/range checks. Frame-relative positions, physical
  mass, geometry/radius and shares pass through narrowing; radius is rounded
  outward when required. [PreparedParticle/Participation](../src/assembly/study.h)
  store mass/radius/share as float even when Point uses double. V2 structural
  recipes own no world starts. Source precision and runtime precision must be
  audited separately; this slice changes no recipe/schema or source values.
- Optional viewport capture reads existing report/body values. Its serialization
  formatting is not evidence of wider engine arithmetic. A future wider live
  profile must extend the readback/capture boundary explicitly.

## Native substrate and finite domain

[Param](../src/engine/runtime.h) already carries explicit dtype. Both initial
and prepared `argument_float` setters accept double; the
[native cache](../taichi/taichi/program/launch_context_builder.cpp) stores f32 or
f64 according to the declared argument. Typed f64 constants and casts/bitcasts
already exist in [IRBuilder](../taichi/taichi/ir/ir_builder.h). Transfers use
ndarray element size. The first slice qualifies these facilities and adds double
to the packed boundary; it does not require a second compiler/transport layer.

The fork advertises [data64 for x64/arm64/CUDA](../taichi/taichi/program/extension.cpp).
That is not a throughput guarantee or complete device capability test.
`MathMode::Strict` disables fast_math in a copied kernel compile config; it does
not change Program defaults or undo the forced FTZ mode in
[program.cpp](../taichi/taichi/program/program.cpp). CUDA also has an explicit
[FTZ module flag](../taichi/taichi/runtime/cuda/jit_cuda.cpp).

Measured CPU finding: narrowing f64 `2^-149` to f32 produces zero under this mode,
although an IEEE gradual conversion can represent it. Host casts after Program
construction can exhibit the same behavior. The new primitive therefore rejects
all nonzero f64→f32 inputs with magnitude below `2^-126`, including borderline
values that could round up to the smallest normal. It returns invalid instead
of silently calling that result legitimate jitter removal. Normal ties, finite
overflow and the rejection domain have independent known-value checks.

Same-container masks preserve stored subnormal encodings; native f32-subnormal
promotion to normal-range f64 is tested separately. Neither result qualifies
arbitrary subnormal calculations. The first slice changes no global floating
mode or software conversion path. Supported full-range mixed arithmetic remains
an explicit later decision rather than an assumed property of 64-bit addressing.

## Resource manifest obligations

[PreparedCapacity](../src/field/prepared_capacity.cpp) currently defaults floating
numerical shapes to four-byte elements. [PreparedStorage](../src/field/prepared_storage.cpp)
and Activity also create explicit f32 arrays/workspaces. A live f64 profile must
derive actual per-array types rather than double total scratch indiscriminately:

| Family | Existing typed extent | Later conversion obligation |
|---|---|---|
| Numerical body/group/share/determiner | 11P / 8G / E / P f32 | Decide the actual stored width of each surface and corresponding host view. |
| Numerical motion/focus/universal | 3P / 6G / 11 / 1 f32 | Separate physical/history/residue fields before any layout change. |
| Update reduction/new-row/inspection | 14G+12 / 12B / 3P+6G / 11 f32, with physical sentinels | Charge actual host/backend copies, new dtype dimensions, transfer and binding payloads. |
| Geometry summaries | 4 f32 native plus host copy; 16 i64 counters plus host copy | Wider focal/contact values must agree with source/body dtype; integer counters and candidate CSR retain their types. |
| Native identity/relationships/activity | Handles, ledgers, i32 CSR/flags and i64 counters | Preserve types and native scheduling; retain old/new peak and lifetime rules. |

P/G/B here use physical `max(capacity,1)` strides where arrays have sentinels.
Host-only metadata, container object overhead and compiler memory are separate
from payload reservations; native pool/root alignment is measured separately.
The new test probes inventory their actual typed bytes and binding payloads.
They add no live candidate banks, LoD metadata or speculative execution budget.

## Integration decisions still open

#177 requires local field matches/centroids at the finest exposed conceptual LoD,
weighted upward compounding through successive exposed comparison levels, and
accounting once within each comparison. A rollup and its components do not create
extra physical mass. Membership paths and construction differ; distinct live
occurrences must remain distinct. Secondary compression remains 1:1.

Audit calculation widths, storage/retained bits and rounding/publication at each
comparison level. A fresh ordered current-member reduction remains a possible
local component; it is not a selected flat all-scale algorithm. Exact matching,
skipped levels, local/inherited force combination, downward whole-object
propagation and native phase ordering still need the bounded integration plan.

Before changing live state, that plan must reconcile accepted body displacement,
centroid contribution/mass accounting, residue discard, old-centre history,
motion/activation and frame transforms. Require independent two/three-level,
multiple-occurrence/path and sleeping/local-change evidence. The present scalar
and transport probes cannot count as that acceptance or as #156/#170 completion.
