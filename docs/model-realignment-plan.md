# Model realignment: conversion and continuation plan

Status: 2026-10-09. Approved model directions are recorded below; this document
plans their implementation and verification. No runtime or database migration
is executed by this documentation change.

## Goal and scope

Prepare the native C++ model and warm-cache bridge to assemble language base
structures and recorded combinations for dictionary ingestion. Preserve the
retained core and usable existing runtime throughout conversion. The next
discussion will define the first English database and formal cross-database
connections; it is a separate design/write decision, not an instruction to
create that database now.

## Governing model

| Concern | Required direction | Still to resolve |
|---|---|---|
| Primary SNode structure | Expresses study-relative conceptual LoD and scale boundaries. | Concrete C++ view/layout mapping. |
| Attached secondary structures | Compression expansion remains at the enclosing conceptual LoD, ratio 1:1 throughout internal dedupe. Environment stays at its existing granularity. | Materialization/reference representation and lifecycle. |
| Conceptual distances | Finest visible conceptual LoD anchors the unit; coarser exposed conceptual levels expand. Start with 64, compare 128 where needed. UTF/character equivalences can omit a step. | Common scalar plus view conversion versus relationship-dependent multiples; state conversion. |
| Object extent | Component-derived spherical enclosure, primarily prepared per centroid/object. | Radial construction, numeric representation and invalidation inputs. |
| Parent response | Proportional contribution on the whole particle; preserve order and repeated occurrences. | Replace the existing share/offset path consistently. |
| Literal cohesion | Scaled molecular distances and higher/lower attraction; no beads-on-string geometry or special shear hold requirement. | Calibration in larger connected models. |
| Whitespace | Empty recorded positions preserve spaces; optional local inert separator in the model. | Typed formatting/reconstruction rules and safe numerical treatment if instantiated. |
| Contact | Test without corona effects; ordinary body contact remains. | Variable-radius contact and comparative results. |

Conceptual LoD, compression depth, body radius and structural mass are distinct
quantities. A deeper storage layout does not multiply distance, mass or force.
No new force law is implied. Keep the ordinary inverse-square relationship;
consistent conversion of positions, centroids, movement and distance must be
derived rather than adding an arbitrary compensation multiplier.

The legal-document example is the defining compression case: a document literal
contains boilerplate references, including nested dedupe. Expanding those
references reveals more of that literal at the same scale. A paragraph or word
does not become a conceptual scale boundary merely because a compressor used
it as a reusable unit.

## Source audit and implementation boundary

Read against main 8c81231f889d1ae737465f8925edbc11688a044f:

- Native `engine/taichi/taichi/ir/snode.{h,cpp}`: child insertion increments
  structural depth; axis shapes accumulate storage/index extents. There is
  no conceptual distance policy in that class. Existing nesting can support
  the proposed organisation, but does not implement the model automatically.
- `engine/tests/engine_smoke_test.cpp`: separate live trees and sparse
  activation are exercised. These tests do not prove conceptual zoom,
  dedupe semantics, shared physical subtrees or physical-scale invariance.
- `engine/src/field/field.h`: current particle payload has positions,
  velocity, mass and force; edges carry share and internal offsets. There is
  no per-particle extent or conceptual-scale field in that payload.
- `engine/src/field/field.cpp`: contact uses a unit-body separation plus
  corona terms and a defined-bond stretch hold. Setting corona to zero alone
  does not reconcile the hold or variable body extents.
- Warm study composition, persistent identity across recomposition and the
  native pool/layout mapping remain open in #106. Existing Taichi pointers
  allocate child blocks of a schema; they are not arbitrary references to
  shared warm definitions.

Recheck these seams at the start of each implementation PR. Keep the Taichi
substrate, physics engine, harness, database/cache manager and WAL manager
separate. Do not implement speculative analyst behaviour.

## Conversion sequence

### 1. Establish the view and identity contract — #106, #115

Specify a minimal C++ representation of primary conceptual boundaries and
secondary compression attachments. The names are model roles, not new Taichi
node types. Prefer existing tree/container facilities where they suffice;
do not modify the Taichi compiler merely to add model semantics.

For each exposed occurrence, the bridge must be able to resolve its cold
identity, live particle identity, enclosing conceptual scale and composition
position. Choose whether these are explicit fields or derived from the
prepared structure after comparing reuse, traversal cost and memory cost.

Secondary compression inherits the primary scale across any number of nested
compression steps. Refocusing a study may express different primary
relationships over the same cold objects. Stable cold identity is preserved;
define particle survival, replacement and reference remapping explicitly.

Specify collapse/expand publication so aggregate and exposed parts cannot
accidentally contribute twice to mass, fields or sibling participation.
Retain repeated occurrences as distinct contributions. Reuse a deduplicated
definition without conflating its multiple live occurrences.

**Exit:** reviewed contract, legal-document fixture design, explicit pool
claim/release and recomposition rules. Distinguish value assignment in a
preallocated pool from Taichi sparse activation/allocation.

### 2. Establish component-derived extent — #120

Specify the centroid-relative enclosing construction before implementing its
formula. Preserve the intended near-equal single/double hex-couplet example
and larger triple/quadruple cases without asserting exact values prematurely.
Resolve how nested compressed components supply their extent at the inherited
scale and how conceptual transitions convert it.

Prepare extent on construction; identify which composition, geometry and
scale changes invalidate it. Reuse the result for unchanged objects.
An enclosure calculation must not restore hidden positional parent response.
Do not derive mass from volume or radius.

Audit unit-sphere assumptions in contact, reach/budget geometry, staging,
upload/download and diagnostics. A fixed slot count is a memory budget, not
proof of one universal physical packing radius when object sizes vary.

**Exit:** agreed geometry examples and deterministic enclosure tests, with
a documented recomputation rule and no per-tick recursive content walk.

### 3. Convert parent response — #118

Remove internal application offsets from force and centroid participation
together. Keep actual positions of exposed particles. Preserve proportional
participating mass and repeated ordered parents; verify how exact structural
counts reach numerical field arithmetic without treating a stored float share
as authoritative archival mass.

Do not add torque, rotational state or directed rotary alignment.
For an unexposed object, evaluate the whole spherical boundary object through
its relevant inputs/outputs.

**Exit:** whole-particle contribution tests, repeated-parent cases and
centroid seeding/publication consistency; payload and callers migrate together.

### 4. Implement distance and compression operations — #106, #115

Use the phase-1 contract to carry inherited scale through prepared SNode
pieces and hot assembly. Compression-only expansion has ratio 1; it changes
representation, not the surrounding measurement frame. A true conceptual
zoom performs the required frame conversion.

Specify conversion of particle positions, centroid state, radii and movement
units, including velocities and braking interpretation, before claiming
equivalent trajectories. Mass/identity/composition are not rescaled.
Derive a canonical comparison frame for verification.

Cache prepared results by all relevant dependencies: study/view, composition
revision, scale choices and exposure state as needed by the selected design.
Invalidate affected derived pieces on change; keep cache-key/schema choices
open until the contract is reviewed.

**Exit:** arbitrary compression depth does not accumulate a distance factor;
conceptual 1 → B → B² and focus-change cases pass with consistent units.
The exact implementation of primary/secondary attachment is documented.

### 5. Reconcile whitespace — #116

Retain representative positions including repeated and leading/trailing gaps.
Dedupe expansion must reproduce those positions without inserting recorded
whitespace tokens or compacting the sequence.

If separator particles are instantiated, restrict them to local higher/lower
connection in their construct. No structural parent/child relationships,
gathering mass or field attraction to other whitespace occurrences. Decide
safe integration treatment explicitly; zero gathering mass is not permission
to divide by zero. Do not claim tabs/newlines are recoverable from untyped
spaces without a formatting representation decision.

**Exit:** sparse-position round trips and nested-dedupe gap fixtures pass;
optional model separators satisfy the participation exclusions.

### 6. Convert contact and run the corona-disabled experiment — #119

Separate ordinary contact from corona spacing and the superseded defined-bond
hold in the current kernel. Use the agreed extents in one measurement frame
for ordinary spherical contact (the sum of the two radii); settle the actual
contact response in the implementation review. Avoid silently introducing
an all-pairs collision pass to replace the existing ledgered bond path.

Test the realigned model without corona effects or a shear-protective hold.
Preserve a reproducible old baseline for comparison, with each experimental
change identified. Do not reinstate the old 1/1.5/2 spacing ladder as a gate.

**Exit:** contact and separation are measured with variable extents, no
unintended corona residuals, and no unexamined singular/overlap handling.
Permanent corona removal is assessed from results, not presumed.

### 7. Calibrate and assemble the ingestion-facing slice

Begin with factor 64; compare 128 and relationship-dependent multiples only
where measurements indicate the need. Measure local cohesion under differential
pull, distant aggregate effects, contact/separation, settling, numerical range,
memory and active work as both connectivity and linear extent grow.

Use C++ fixtures first, then prepared language base structures and recorded
combinations. A dictionary ingestion slice must preserve provenance, ordered
composition, repeated occurrences, sparse positions and reusable identities.
Demonstrate warm assembly and selected hot exposure without requiring a
complete analyst or a fully expanded universe tree.

**Exit:** publish results, chosen defaults, remaining hypotheses and the next
bounded implementation task. Do not make every long-term calibration question
a blocker for preparing language records.

## Verification matrix

| Fixture | Required observation |
|---|---|
| Legal document with nested deduplicated boilerplate | All secondary expansion steps inherit the document's scale; surrounding scale/coordinates are not rebased solely by expansion. |
| Same literal with different compression partitions | Same expanded content, order and gaps; no scale or mass inflation caused by compressor nesting. |
| Repeated boilerplate in one/two documents | Definition reuse preserves distinct occurrences and their participation; no accidental shared mutable particle state. |
| Mixed primary and secondary hierarchy | Only explicit conceptual boundaries apply non-unit scale transitions. |
| UTF endpoint / character equivalence | No forced full distance step for a naming equivalence. |
| Conceptual zoom and two study focuses | Identity retained; transformed state agrees in a common comparison frame within declared numerical tolerances. |
| Collapse then expand | No duplicate aggregate/child counting; deterministic reconstruction and valid references. Exact force equivalence requires an explicit aggregation contract, not dedupe alone. |
| Size examples | Agreed single/double/triple/quadruple enclosure cases; body size independent of structural mass accounting. |
| Whitespace | Gaps survive compression and expansion; optional separator obeys local-only participation. |
| Small and larger connected models | Cohesion, shear, distant fields and contact evaluated without a corona or artificial construct scope mask. |
| CPU then available CUDA | Same declared invariants and tolerances; record unavailable device coverage honestly. |

Do not assert that adding exposed components leaves every resultant force
unchanged: exposure may reveal interactions. The 1:1 invariant concerns the
distance frame, not automatic equality of coarse and detailed dynamics.

## Delivery and compatibility

Deliver focused PRs: contract/fixtures; extent; whole-particle response;
scale/bridge; whitespace; contact experiment; calibration report. Extent and
response can be developed after the contract independently, but integrated
contact testing depends on extent and scale. Update tests, payload readers,
writers and documentation in the same behavioural PR. Preserve a runnable
intermediate state and explicit version compatibility for any changed staging
or saved-state format.

Record baseline settings and retain previous runs/commits for rollback.
No destructive core rebuild, history rewrite or silent migration is part of
this plan. Cold schema changes require a demonstrated need; primary/secondary
roles initially belong to study-oriented working composition.

## Database continuation and next discussion

The record-integrity workstream proceeds alongside model conversion. PR #111
remains open at head 5120ee6e6316bf76b6cd7a91ec51913054464877 and is currently
not mergeable. Its previously recorded command-atomicity, restart validation
and restore-guidance findings still require resolution/review; this plan does
not clear them. Accepted plans #108 and #112 are merged, not evidence that
their executions are complete. Previously acknowledged cross-encoding coverage
work stays deferred for MVP.

For the English/cross-database discussion, prepare decisions on:

- English store scope, provenance and first bounded dictionary source;
- reuse of core encoding identities and qualification/routing of references
  across databases without accidental local/global privacy crossover;
- ownership of forward/reciprocal links, command transaction boundaries,
  deferred WAL obligations and idempotent retry/reconciliation;
- language base structures and recorded combinations to assemble first;
- a read-only core inventory and a reviewable additive first-ingestion plan,
  including restore/recovery evidence and validation totals.

Do not open or populate English, choose a final cross-database schema, or
create formal cross-database links from this document alone. These are the
next discussion. Model calibration need not prevent that discussion, but
durable writes need the relevant record-integrity prerequisites.
