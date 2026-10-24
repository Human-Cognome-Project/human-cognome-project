# Current-state upward centroid integration

Status: **proposed bounded plan for review, 2026-10-24**.
Owners: [#177](https://github.com/Human-Cognome-Project/human-cognome-project/issues/177)
and [#172](https://github.com/Human-Cognome-Project/human-cognome-project/issues/172).
Baseline: `main` at `798b49a`, following accepted #178 and the #179 clarification.
The [precision primitives](../engine/docs/NATIVE-PRECISION.md) are built; the
[state audit](../engine/docs/PRECISION-STATE-AUDIT.md) identifies the later live
conversion. This proposes its next component, not a complete live profile.

## Proposed first slice

Add a bounded **upward centroid candidate operation on current backend data**.
Evaluate an explicitly supplied comparison hierarchy from its finest local
cohorts until one final centroid remains, at whatever scale that hierarchy defines.
Candidate reductions/outputs use f64, with optional experimental significand
trimming at declared expressed-comparison publication boundaries. Existing body
coordinates remain f32; widen before products, offsets and sums. This gains
reduction precision without claiming to recover lost body-storage detail.

The operation installs no outputs into the existing f32 fields. It does not move
bodies, apply downward forces, change focus, edit topology or consume/wake activity.
It is a current-state numerical component of later live integration, not a second
solver or another source of physical state. Existing ticks and coherent updates
remain intact. #156/#170 admission and candidate-tick publication remain outstanding.

Acceptance releases this first candidate slice only. Proposed review decisions:

1. Start with an explicit fixed hierarchy and whole-body contributions; reject
   fractional participation and ambiguous overlapping child ownership.
2. Read current resident positions/masses, including sleepers, at the existing
   exclusive synchronized Harness boundary.
3. Use one operative coordinate frame. Internal aggregation adds no distance
   multiplier; a hierarchy edge alone is not a conceptual LoD transition.
4. Separate internal grouping from expressed comparison publication. Combine
   temporary partial reductions before deliberate trimming. After publication,
   parents use the child's published centre, not its discarded positional detail.
5. Evaluate on explicit request with prepared resources. Do not select a force
   equation, semantic matching policy or persistent activity scheduler here.

## Comparison identity and contributions

Preparation receives bounded node/edge tables, canonical source order, selected
physical occurrence handles and explicit comparison contexts. A context identifies
the requested study/cohort, exact source field identity and expressed conceptual
level. Equal labels in different contexts do not automatically mean one calculation.
The owning input supplies this metadata; the operation discovers no semantic
matches and derives no hierarchy from raw SNode depth. Compression/naming stays 1:1.

Bind particle/field references to full native handles and a committed generation.
Validate selected finest-cohort membership against its referenced fields and require
whole-body participation, share one. Read coordinates and mass from current backend
values, not caller starts/weights or stale host staging. Source ledgers/revisions
and parent ordinals remain unchanged.

Coalesce identical membership-path aliases per field/occurrence within a comparison
context. Distinct native occurrence handles remain distinct even when they instantiate
one definition. Do not deduplicate distinct parent ordinals as aliases: requests
needing fractional parent-value accounting are unsupported in this first slice.
A duplicate is a validated alternative path to the same contribution, not merely
an endpoint collision. If distinct source contributions would demand one body's
whole mass twice within a comparison, reject that unsupported accounting.

Children of a parent must represent disjoint physical occurrence cohorts whose
union matches its explicitly selected cohort. A body contributes once within that
comparison. A rollup summarizes those bodies; it adds no independent physical mass.
Reject overlap rather than silently reassigning mass, guessing overlap corrections
or flattening the hierarchy into an all-scale leaf reduction. General overlapping
membership graphs remain a separate #177 matching/accounting decision.

Validate acyclic dependencies, context/level transitions, one final root and complete
selected-cohort coverage before dispatch. Internal grouping nodes belong to their
existing comparison; expressed comparisons advance through the declared hierarchy.
A nested field has a distinct context at every level where it is expressed.
Stop at the single final centroid; do not force a fixed number of LoDs or add a
preferred global root. That root must be an expressed comparison publication.

Evaluate only the explicitly supplied exposed comparisons. Skipped/concealed
levels add no synthetic centroid or rounding stage; a declared direct dependency
can lead to the next expressed comparison. This does not implement exposure
selection/exchange or infer hidden intermediate fields.

An empty prepared study returns Empty with no physical centre. A nonempty request
with an empty required cohort, missing root, stale/foreign/dangling reference,
unsupported share or conflicting ownership rejects without changing live state.

## Numerical and scale boundaries

Use an ordered relative weighted mean for positive finite contributing masses:

```text
M = sum(m_i)
c = a + sum(m_i * (x_i - a)) / M
```

Reference a is the first contributor in canonical order in the same operative
frame; it is a calculation reference, not placement. Finest inputs are current
body coordinates/mass. Coarser expressed comparisons receive published child
centres and their cohort masses. Promote native loads before arithmetic. Mass is
an untrimmed f64 sum, not a quantized positional value. Reject unsupported or
nonfinite arithmetic before making the candidate inspectable.

Internal grouping may organize partial mass/relative-offset work for locality.
Its scratch is temporary within this evaluation, not another physical centroid
or publication boundary. Do not deliberately trim at every tree edge. Measure
association/cancellation effects against an independent ordered reference;
arbitrary regrouping is not promised bitwise identical in floating point.

At an expressed comparison boundary, finish the combined local result, then apply
that context's output policy once. The first profile is f64 calculation/f64
candidate storage: full fraction precision or explicitly requested experimental
Q_b from #178. Other profiles reject explicitly in this slice. This selects no
production rounding rule or LoD precision ladder. Independent diagnostics may
use f32 where their precision is sufficient.

A trimmed child's trial-to-published difference is discarded. Parents consume
its published centre and untrimmed cohort mass, never a hidden unquantized first
moment or rounding remainder. Partial sums before publication are temporary
arithmetic; discarded detail after publication cannot return at the next comparison
or a later evaluation. Recompute from current accepted body state each time.

The staged result is intentionally resolution-dependent. Compare with a hierarchical
reference applying the same declared publication boundaries, not a flat unquantized
mean. Quantized child centres need not preserve that flat mean; physical contribution
and mass accounting must remain correct. Report trial/published centre, mass and
experimental retained precision at each expressed boundary.

All candidates use the current operative frame and unchanged double frame anchor.
Resident bodies already include declared source-unit conversion. Do not multiply
those positions again on internal grouping or upward aggregation. Explicit conceptual
LoD transitions retain their declared unit/context mapping; source conversion,
centroid aggregation and force-distance scaling are separate operations. Existing
ScaleBinding fixture conversion may supply examples, but its historical factor 64
does not establish a new universal calibration rule.

For example, source positions 1 and 1 with declared units 1 and 64 become resident
positions 1 and 64. With equal masses their mean is 32.5. Additional same-LoD internal
grouping must not apply another factor 64, and compression/naming remains 1:1.
This checks conversion exactly once, not the still-unimplemented force-distance
transition policy under #115/#177.

## Native boundary and prepared resources

Attach the operation to prepared field execution, sharing the current native maps,
body/participation arrays and ownership checks. Prepare generic bounded kernels,
bindings and candidate buffers before measured queries. Bind the explicit hierarchy
in reserved tables at a synchronized topology boundary. Preparation is separately
measured work, not per-query allocation.

Evaluate finest-to-root dependencies through prepared level/CSR tables and ordered
reductions. Do not add an SNode layout per level, custom queue or second awake set.
This explicit read-only calculation includes sleepers; it does not poll/integrate
unused potential V. Dirty propagation and native per-tick phase integration remain
later work, not a hidden scheduler in the on-demand operation.

Bound hierarchy nodes, links, cohort references, levels, scratch and work independently
alongside existing P/G/E limits. Charge owned references and scratch in the existing
reservation accounting as appropriate. Inventory concrete host headers/handles,
validation/cohort expansion, node/child/context/profile tables, f64 candidate
centres/masses, temporary relative partials, status, inspection, actual bindings
and transfer bytes. Integer identities/indexes/flags retain their widths.

Topology validation/deduplication may cost more than the ordered evaluation's
`O(selected contributions + hierarchy links + nodes)` work. Bound/measure preparation
separately; do not call it all declaration-only work. Distinguish setup/empty warm
dispatch, first nonempty query, ordinary repetition, inspection and rejection.
No automatic capacity expansion, host restore or f32 fallback is provided.

Results use the existing context, topology generation, completed-tick observation
and a query identity. These are provenance, not scheduling epochs. Successful
ticks, source edits, hierarchy rebinds and superseding queries invalidate old
observations. Rebinding after source edits is explicit; a stale hierarchy must not
silently follow recycled records. The next valid query reads current backend values.

Only a fully validated candidate becomes inspectable. Ordinary resource, reference,
hierarchy/profile or arithmetic rejection leaves readiness, particles, existing
centroids/focus, source topology, native activity/history and Demand unchanged.
Unexpected backend/invariant failure marks execution unready under existing policy.
This introduces no healthy partially published physical state or recovery path.

## Acceptance and remaining live integration

Required bounded CPU evidence:

- One, two and three expressed comparison levels, same-LoD internal grouping and
  one final root. Independently check per-level centres/masses and contribution
  ownership, including skipped unexpressed levels, unequal positive masses and
  independent repeated definitions.
- Membership-path aliases coalesce per context while distinct copies remain
  independent. Reject fractional participation, overlapping children, cycles,
  invalid levels/roots and stale/recycled/foreign references. Preserve original
  ledgers/ordinals and physical state.
- Demonstrate useful f64 reduction detail beyond a narrowed f32 centre. Exercise
  cancellation/order, combined contributions, full precision and experimental
  trimming, same-cell wobble, boundary crossing and origin bias on both signs.
  Discarded child detail must not return at a parent or a repeated evaluation.
- Declared source-unit transition converted once; same-LoD grouping adds no distance
  multiplier, with compression/naming at 1:1. Separate this from force-distance policy.
- Read current values after existing real ticks, including sleepers and changed
  local contributions. Poison BulkCopy staging to demonstrate backend reads.
  Compare complete pre/post numerical/control/source/Demand state on successful
  queries and ordinary rejection; no survivor reseed.
- Both CPU host paths, empty/first use, holes/final records, bounded maximal demands
  and independent node/link/reference/level/scratch/work exhaustion. Retain backing,
  layout/compiler counters and recurring C++ allocation observations with their limits.

Run appropriate native regressions. Retain source/compiler/backend pins, actual
architecture, profiles, raw results, manifests and independent oracles. No database
change, Python runtime or immediate GPU qualification is part of this slice.

Afterward #172/#177 still require the bounded live conversion: fractional/overlapping
field matching, useful f64 body storage and numerical interfaces, accepted-motion
accounting, coherent centroid/history replacement, local/inherited force combination,
downward whole-object response, native phase/wake order and supported frame transforms.
Production rounding and per-LoD calibration remain open.

Do not relabel candidate-only results as that live conversion. Do not clear the
mixed physical/residue group_delta, replace existing field centres, invent same-tick
recursion or integrate bodies twice. #170's candidate motion/contact transaction
must be satisfied before accepting the corrected operational tick. #156/#145 retain
their full CPU operation exit and later actual-hardware qualification.
