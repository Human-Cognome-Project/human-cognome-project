# View and identity contract (Phase 1)

Status: 2026-10-10. **PROPOSED / DRAFT contract, pending review.** Design intent
only. Nothing in this document is built, and no runtime, Taichi source, database or
saved-state change accompanies it. Where a choice belongs to Patrick it is listed
under [Open decisions for review](#open-decisions-for-review) with a recommendation,
not settled here.

Implements step 1 ("Establish the view and identity contract") of the
[model realignment plan](model-realignment-plan.md). Tracked in
[#106](https://github.com/Human-Cognome-Project/human-cognome-project/issues/106)
(data bridge, pool identity) and
[#115](https://github.com/Human-Cognome-Project/human-cognome-project/issues/115)
(operative scale, zoom rebasing). The plan's Governing model, Source audit and
Verification matrix are inherited unchanged; this document does not re-decide
anything they settle.

Revision 2 (2026-10-10) after independent validation: occurrence identity is now
view-independent (section 3.2), token location and claim generations are reopened as
decisions, groups and the harness-construction consequence are addressed, and gap
observations are deferred to #116.

Plan exit criteria for this step: a reviewed contract, the legal-document
fixture design (read here as structural fixtures, with the legal document as the
defining in-place-nesting example), and explicit pool claim/release and recomposition
rules. This document is the draft of all three; "reviewed" is not yet true.

## Location

`docs/` is used because the contract spans three components that
[AGENTS.md](../AGENTS.md) keeps distinct (the engine's field/pool, the warm-cache
bridge, and the Taichi substrate) and sits beside its governing plan. Component docs
that depend on it (`engine/docs/ACTIVE-FIELD-MODEL.md`,
`kernels/database/WORKING-SET-AND-LEDGER.md`) should link here once it is reviewed.

## Boundaries of this contract

- It defines roles, identifiers and rules. It does not define a final C++ API, a
  cache-key or schema, a conversion formula, or a calibrated factor.
- It adds no new Taichi node type and requires no change to the Taichi compiler.
- It adds no force law, no mass derived from volume or radius, no torque, rotation or
  rotary alignment, and no compensation multiplier. The inverse-square law is
  unchanged.
- The harness (control interface), the cache/database manager, the WAL manager and the
  Taichi substrate remain separate. Where this contract names a "bridge", that is the
  warm-to-hot data bridge tracked in #106, not any of those components.
- Analyst functions do not exist; none are specified or assumed.
- Whitespace separators (#116), component-derived extent (#120), whole-particle
  response (#118) and contact (#119) are separate plan steps. Only their interfaces
  to this contract are noted.

## Source audit

Read at branch `docs/view-identity-contract`, based on main `dc0db21`. The plan
audited main `8c81231`; the seams below were rechecked here. Status column uses the
repository's BUILT / planned / open vocabulary.

| Seam | Evidence | Status |
|---|---|---|
| Particle payload is packed SoA: positions, velocity, mass, force; element `f` of item `i` at `f * count + i` | `engine/src/field/field.h:24-30`, `:111-118` | BUILT |
| Hot field arrays are Taichi `Ndarray`s, not an SNode tree | `engine/src/field/field.h:157-164` | BUILT |
| Particle, group, edge and bond counts are fixed at `Harness` construction | `engine/src/field/field.h:87-91`, `:93-96` | BUILT |
| An edge is `(kEdgeParticle, kEdgeGroup)` ints plus `kShare` and `kOffX/Y/Z` floats; the particle reference is a bare array index | `engine/src/field/field.h:41-52` | BUILT; offsets are withdrawn from the intended model (#118) |
| Edge and bond indices are range-checked at upload | `engine/src/field/field.cpp:665-681` | BUILT |
| The centroid pass weights position (plus offset) by `mass * share` per edge | `engine/src/field/field.cpp:616-625` | BUILT; offset path pending #118 |
| Universal mass sums `kMass` over every pooled particle each tick | `engine/src/field/field.cpp:155-158` | BUILT |
| Integration divides `dt` by `kMass` with no mass-zero gate | `engine/src/field/field.cpp:463` | BUILT; see [0x hazard](#4-the-0x-slot-in-the-fixed-pool) (from reading, not run) |
| Group publication guards its divisor with `kWeightFloor` | `engine/src/field/field.cpp:47`, `:645-656` | BUILT |
| Contact is the corona separator plus the defined-bond stretch hold, driven from bond edges | `engine/src/field/field.cpp:261-275`, `:293`, `:343` | BUILT; superseded direction (#119) |
| No per-particle `token_id` and no generated sibling field in the field arrays | `engine/docs/ACTIVE-FIELD-MODEL.md:133-139`; `kernels/database/WORKING-SET-AND-LEDGER.md:112-115`; no `token_id`/`particle_id` in `engine/src/field/` | open |
| `SNode::id` is a schema node id from a counter; `depth` is structural depth | `engine/taichi/taichi/ir/snode.h:88-90`; `snode.cpp:12`, `:220` | BUILT (upstream) |
| Child insertion creates a child at `depth + 1` | `engine/taichi/taichi/ir/snode.cpp:14-17` | BUILT (upstream) |
| Tree id is separate from node id | `engine/taichi/taichi/ir/snode.h:342`, `:353` | BUILT (upstream) |
| `pointer` and `place` are fixed per-container schema types; `need_activation()` marks `pointer`, `hash`, `bitmasked`, `dynamic` | `engine/taichi/taichi/ir/snode.h:166`, `:264`; `snode.cpp:268` | BUILT (upstream) |
| Pointer activation allocates a child block; deactivation recycles it; an inactive cell reads the ambient element | `engine/taichi/taichi/runtime/llvm/runtime_module/node_pointer.h:41-60`, `:77`, `:90-96` | BUILT (upstream) |
| Smoke test exercises separate live trees and scattered pointer activation | `engine/tests/engine_smoke_test.cpp:90-99`, `:167-200` | BUILT |
| Nothing exercises conceptual zoom, dedupe, shared subtrees, pool claim/release or scale invariance | absence of any such case in `engine/tests/engine_smoke_test.cpp` (`check_one_arch`, `check_sparse`, `check_configuration`); `engine/tests/field_test.cpp` not audited line-by-line for this | open |
| Fixed particle pool, claim/release as declaration, `0x` as inert null | `engine/docs/OPERATIONAL-PLAN.md` section 3.11 | model intent; not implemented as a pool allocator |
| Shared subtree is a schema/DB property; each active cell owns its child storage | `engine/docs/taichi-snode-mechanics.md:101-128` | reference |

Limits of this audit: the `0x` mass-zero consequence at `field.cpp:463` and the
absence of a pool-claim test were established by reading, not by running a kernel or
searching every test file exhaustively.

## 1. Identifiers and roles

Four things are kept distinct. None may stand in for another.

| Term | What it is | Scope and lifetime | Where it lives |
|---|---|---|---|
| `token_id` | Reusable represented object in the record library (cold identity). Many particles may share one. | Permanent in the cold store. | Cold store; carried by the bridge as a reference. |
| `particle_id` | One active instance of a token in the working model. | One claim of one pool slot (see section 3). | Bridge tables; its engine index is the slot index in the hot arrays. |
| `SNode::id` | Taichi's structural (schema) node identifier. | Monotonic within a `Program` lifetime; spent by recomposition. | Taichi substrate only. |
| SNode tree id | Taichi's identifier for a registered tree. | Recycled on `destroy_snode_tree`. | Taichi substrate only. |

Two further model terms are introduced as **roles**, not as Taichi node types and not
as new schema:

- **Primary boundary:** a point in the study's primary structure at which the
  conceptual level changes (or is declared an equivalence). It owns a **scale frame**.
- **Secondary attachment:** a compression/dedupe definition referenced from within one
  scale frame. It owns no scale of its own.

An **exposed occurrence** is one place in the study where a token is live, either as a
claimed particle or as part of an expanded definition about to be claimed. An
occurrence is the unit the bridge resolves.

## 2. Minimal C++ representation

### 2.1 Principle

Primary and secondary roles are recorded in plain C++ tables prepared by the
warm-cache side for a study (struct-of-arrays, in the same idiom as `field.h`), not in
Taichi node types. An SNode tree may be used wherever a layout is wanted for storage;
it is never the carrier of conceptual scale, and `SNode::depth`, `SNode::id` and tree
nesting are never read to derive scale (plan, governing model: storage depth does not
create a scale step).

Rationale for tables over SNode-as-model:

- Taichi gives no conceptual-scale policy (`snode.cpp:14-17` only increments
  structural depth).
- Taichi cannot alias one physical subtree under several parents
  (`taichi-snode-mechanics.md:101-128`), so a reusable definition cannot be a shared
  physical subtree anyway; it must be a warm definition referenced by tables and
  materialized per occurrence.
- No compiler change is required.

### 2.2 Records (illustrative shape, not an API)

```text
ScaleFrame            -- one per primary boundary in a view (a boundary is the point in
                         the primary structure; its frame is the record that carries scale)
  frame_id            -- bridge-local, view-scoped
  step_k              -- integer: expressed conceptual steps above the finest visible frame
  equivalence_of      -- optional frame_id this frame names without a step (Δk = 0)

Occurrence            -- one per exposed (or about-to-be-exposed) occurrence
  occurrence_key      -- VIEW-INDEPENDENT identity, anchored in the composition (see 3.2)
  token               -- reference to the cold token_id
  frame               -- ScaleFrame the occurrence lives in (view-scoped)
  parent_occurrence   -- enclosing composite occurrence in the composition
  ordinal             -- composition position under that parent (data, not a distance)
  state               -- exposed (has a live slot) | collapsed-into-aggregate | not exposed
  slot                -- engine index, valid only while exposed

SecondaryAttachment   -- one per use of a deduplicated definition
  attached_frame      -- the frame whose scale it inherits; never a step of its own
  definition          -- reference to the warm/cold definition (reused, not copied)
```

No storage or nesting depth is recorded in these tables, so nothing can read it as
scale. Where a test needs the depth of a dedupe chain it counts it from the
definitions, outside any scale calculation. The names above are model roles for
discussion; the record shapes are illustrative, not an API.

`token` is a reference to the cold `token_id`; its hot width and form are open
(decision D9). `step_k` is an integer, not a stored `B^k`: the factor `B` (64 first,
128 candidate) is a view parameter, so changing the candidate factor edits no
per-occurrence data.

### 2.3 Explicit fields versus derived-from-structure

Compared for each of the four quantities the bridge must resolve per exposed
occurrence. "Traversal" is cost to answer the question with no stored value.

| Quantity | Explicit per occurrence | Derived from structure | Recommendation |
|---|---|---|---|
| Cold identity (`token_id`) | One token reference per occurrence. Required anyway: the sibling field groups live occurrences by token and the field arrays hold none (`WORKING-SET-AND-LEDGER.md:112-115`). | Not derivable once a definition is reused: the definition names its content but not which live occurrence is which. | **Explicit.** |
| Live particle identity (`particle_id`) | A slot index per exposed occurrence. | Not derivable: it is claim state. | **Explicit** (this is the engine index; section 3). |
| Enclosing conceptual scale | One frame reference (a small integer) per occurrence, resolved to `step_k` by one lookup in the few frame records. | Walk up parents to the nearest primary boundary: cost proportional to storage/secondary depth, which is exactly the quantity that must not matter, and it would make nested dedupe look costly. | **Explicit frame reference; `step_k` held once per frame, not per particle.** Zoom rebasing then edits the frame records (few), not every occurrence. |
| Composition position | Parent occurrence plus ordinal per occurrence. | Re-derive from the definition and the path: cheap for one occurrence but needs the reused definition and path every time, and repeated occurrences differ only by ordinal. | **Explicit parent and ordinal**, because they are already stored cold (`token_parent` carries ordinal, `WORKING-SET-AND-LEDGER.md:97-99`) and are small. |

Reuse: explicit `frame` is what makes a deduplicated definition reusable at any scale
frame; the definition itself holds no scale. Memory: the per-occurrence record above has seven
fields (key, token, frame, parent, ordinal, state, slot). All but the key are fixed
width and independent of dedupe depth. The key is a path and so, taken literally, grows
with composition depth; keeping per-occurrence memory depth-independent therefore
needs a compact handle for the key (decision D6). The qualitative claim is that
nothing here grows with dedupe nesting; no sizes were measured. Traversal: scale resolution is O(1); no per-tick
recursive content walk (cf. the plan's extent exit criterion).

Hybrid recommendation, stated plainly: **explicit identity and position, explicit
frame reference, frame-held scale.** Pure derivation of scale by walking is rejected
because its cost tracks the quantity that must not influence scale.

The cache key and schema for prepared views stay **open** (plan step 4).

## 3. Particle identity across recomposition

### 3.1 Why the engine index is stable

In the built harness, particle state lives in packed arrays indexed by slot, and edges
and bonds name particles by that slot (`field.h:24-30`, `:52`, `:57`). These arrays
are `Ndarray`s created once (`field.h:157-164`); registering, replacing or destroying
SNode trees (`add_snode_tree`) does not move them. So **the engine index of a claimed
particle is independent of SNode-tree recomposition.** SNode schema ids and tree ids
change under recomposition and are therefore never used as a particle's identity or
as a reference stored in edges.

If a future pool is backed by a dense SNode container instead of the `Ndarray`s, the
same rule holds: a particle is addressed by its cell index in the preallocated
container, not by the container's `SNode::id` or tree id. Which backing is used is
undecided (see `taichi-snode-mechanics.md:145-155`); this contract is valid for both.

### 3.2 Contract

- **C-ID1 (recommended; decision D2).** `particle_id` is the pool slot index for the
  duration of one claim. A claim generation, if used, makes a stale reference to a
  released-and-reclaimed slot detectable. Whether `particle_id` is the slot or an id
  with an indirection table, and where a generation lives, are open (D2).
- **C-ID2 (recommended; decision D13).** Each live particle needs a `token_id`
  association. Where it lives is open: `OPERATIONAL-PLAN.md` section 3.11 describes
  claim as setting token_id, mass and position in the slot, while
  `ACTIVE-FIELD-MODEL.md:133-139` records that the built arrays hold no token mapping.
  Recommendation: a bridge-side table indexed by slot, leaving the device arrays
  unchanged. The contract's rules hold for either placement.
- **C-ID3.** `SNode::id` and the tree id never appear as `token_id`, `particle_id`,
  or an edge/bond endpoint.
- **C-ID4.** Occurrence identity (`occurrence_key`) is **view-independent**. It is
  anchored in cold or warm composition coordinates: the cold `token_id` of the
  occurrence plus its position along the chain of enclosing cold or warm composites up
  to an **anchor composite**. The cold `token_parent` table is type-level: each row stores a composite
  `token_id`, an `ordinal` and the constituent (`parent_token_id`) at that position
  (`kernels/database/schema/schema.sql:144-171`;
  `WORKING-SET-AND-LEDGER.md:97-99`). It stores no parent *occurrence*; the occurrence
  chain is derived from those rows plus the anchor. `schema.sql` also flags the ordinal
  base (0 or 1) as an open test slot, so D6 and D12 must not assume a base. The anchor
  is a property of the warm composition (the composed set of objects the study
  assembles, which may include composites that exist only in memory), never of the current focus, zoom or exposure. Two
  uses of one deduplicated definition therefore have different keys (different
  enclosing occurrence or ordinal), and repeated siblings are distinguished by
  ordinal. Focus, `step_k`, exposure state and frame never enter the key. How the
  anchor is chosen, and what happens when the composition itself changes, are
  decisions D6 and D12.
- **C-ID5.** A particle **survives** a refocus if and only if an occurrence with the
  same `occurrence_key` is exposed in the new view (refocusing changes the view's
  focus, not the keys). It keeps its slot, its claim
  generation (if the D2 design uses them), and its dynamic state (velocity, force accumulators reset per tick as
  usual), and its position is converted into the new frame (section 5). Mass is not
  rescaled.
- **C-ID6.** A particle is **replaced** when the same occurrence's exposure state
  changes (collapse to aggregate, or expand into parts). The old claim is released
  and the new claims are made in one publication (section 6); no slot is reinterpreted
  in place as a different occurrence.
- **C-ID7.** **Reference remapping.** Because survivors keep their slots, edges and
  bonds between survivors need no remapping. Edges and bonds touching released slots
  are removed; new edges name new slots. If a pool is ever re-laid-out, a single
  `old_slot -> new_slot` map is applied to all edge and bond endpoints in one step and
  validated with the same range check as `validate_indices` (`field.cpp:665-681`).
  Re-layout is not part of normal recomposition and is not designed here.
- **C-ID8.** Stable cold identity is preserved by construction: recomposition changes
  bridge tables and slot claims; `token_id` records are not rewritten and no cold
  write is implied.

## 4. The 0x slot in the fixed pool

Model (Patrick, issue #106; `OPERATIONAL-PLAN.md` section 3.11): an unused pool slot
is `0x`, mass zero, with no inherent presence or location; claiming assigns values to
an existing slot, and releasing zeroes them back.

### 4.1 Representation

- **C-0X1 (recommended; decision D13).** A free slot is a slot whose token association
  is the `0x` token and whose device mass is exactly zero. With the recommended
  bridge-side token table, that table is authoritative for *which* token and the device
  mass is the physical consequence; if the token is instead placed in the slot, the same
  state is read from there.
- **C-0X2.** The free set is a bridge-side structure (a free list or bitmap). It is
  not discovered by scanning device arrays.
- **C-0X3.** Release zeroes position, velocity and force and sets mass to zero, and
  removes edges and bonds naming the slot. Claim assigns token reference, mass,
  position and velocity into the slot. Both are value assignment into storage that
  already exists: no allocation, no schema change, no change in array size.
- **C-0X4.** An invariant is tested at upload: device mass is zero if and only if the
  token association is `0x`. A claimed real token has nonzero structural mass (the seeded
  encoding floor assigns mass 1 to hex atoms and mass 0 only to `0x`;
  `kernels/database/HANDOFF.md:5-8`).

### 4.2 Distinct from an inactive Taichi sparse pointer

| | `0x` pool slot | Inactive `pointer` cell |
|---|---|---|
| Layer | Model/data state in a fixed preallocated pool | Taichi runtime storage state |
| Has an index and storage | Yes, always; visited by every particle loop | No child block; reads resolve to the shared ambient element (`node_pointer.h:90-96`) |
| Becoming live | Assign values to the existing slot (declaration) | `Pointer_activate` calls `alloc->allocate()` for a child block (`node_pointer.h:56`) |
| Becoming free | Assign zeros | `Pointer_deactivate` recycles the block (`node_pointer.h:77`) |
| Identity | Slot index (`particle_id` while claimed) | None as an instance |
| Cost model | Declaration, near-free | Real allocation per activation |

Consequences: pool claim/release must not be expressed as pointer activation;
sparse containers, if used at all, are a separate layer (decision D8) whose
allocation cost must be accounted for or measured at the time, which this contract
does not do.

### 4.3 Requirement on the field kernels (from reading, not run)

The model says nulls are inert with no special-casing (`OPERATIONAL-PLAN.md` 3.11).
In the built integration pass, the mass divide `dt / kMass` has no mass-zero gate
(`field.cpp:463`); a free slot read there appears to yield non-finite velocity. Every
particle loop also runs over the whole pool (e.g. `field.cpp:155-158`). The contract
therefore requires that a later phase make zero mass numerically inert in the same
branchless-gate style the force kernel already uses for zero separation
(`field.cpp:218-226`), and add a test, **before** any free slot is held in a live
pool. This document does not change the kernel.

## 5. Operative scale, carried separately

### 5.1 Representation

Operative scale is the integer `step_k` held on each `ScaleFrame`, resolved through
the occurrence's frame reference. It is separate from `parent_occurrence`/`ordinal`
(composition), from address/storage depth, and from SNode nesting; none of those is
an input to `step_k`.

Per #115: the finest visible frame has `step_k = 0` (one particle is one distance
unit); each less granular expressed frame is a factored expansion, so a frame at
`step_k = k` has operative scale `B^k` with `B = 64` first and `128` a candidate.
Conversion of the numerical state is derived from `step_k` and `B` in Phase 4; it is
not stored per particle.

### 5.2 Rules

- **C-SC1. Conceptual boundary.** Crossing a primary boundary from one frame to the
  next coarser frame increases `step_k` by one.
- **C-SC2. Equivalence boundary.** A primary boundary declared an equivalence
  (UTF endpoint to character in use is the defining example) increases `step_k` by
  zero. It is recorded (`equivalence_of`) so the naming change is retained, but it
  creates no scale step.
- **C-SC3. Zoom rebase.** Exposing a finer level inserts a new frame at `step_k = 0`
  and adds one to every existing frame's `step_k`. Because scale lives on frames,
  this touches the frame records only. Identity, composition (`parent_occurrence`,
  `ordinal`) and cold identity are unchanged. Conversion of surviving particles'
  positions, centroids, radii and movement is Phase 4 work; mass, identity and
  composition are not rescaled (plan step 4).
- **C-SC4. Scale is view state.** `step_k` belongs to the view, never to a cold
  object or to a particle's identity, so two study perspectives over the same cold
  objects can assign different values to the same object. This is the study-relative
  distance required by #106.
- **C-SC5. The multiplier is not fixed here.** Whether the factor between frames is
  one common scalar converted through the view, or additionally depends on
  relationship kind (degrees of separation), is open in #115. The contract
  fixes neither where such a multiplier would live (relationship kinds attach to object
  pairs and may belong to warm-cache SNode definitions, #115/#106) nor any value,
  layout or formula. It reserves only the existence of an as-yet unlocated lookup, and
  leaves its placement to decision D5.

### 5.3 Secondary compression inherits 1:1

- **C-SC6.** A `SecondaryAttachment` has no `step_k`. Every occurrence produced by
  expanding it, at any nesting of dedupe, receives the `frame` of the occurrence it is
  attached under.
- **C-SC7.** Storage depth and secondary-tree depth are not recorded in the view
  tables and no scale, distance, mass or force computation may read them. A test must
  be able to vary nesting depth with identical results (section 8).
- **C-SC8.** Expanding or collapsing a secondary attachment changes representation and
  exposure only. It does not insert or rebase a frame, and does not alter any existing
  `step_k`. Names such as paragraph, phrase or character create a boundary only if the
  primary structure explicitly declares one.
- **C-SC9.** The 1:1 rule concerns the distance frame. It does not promise that
  coarse and exposed-detail dynamics are equal; exposure may reveal interactions
  (plan, Verification matrix note).

### 5.4 Two distinct LoD operations (Patrick, 2026-10-10)

Two operations change level of detail. They are named here because they must not be
confused; they map onto the primary/secondary roles already defined, with no new role.

- **Meta-structure LoD** acts on the meta, primary conceptual structure: the macro
  perspective and overall study framing. Primary boundaries and their frames express
  it. Conceptual zoom, refocus and the expressed-scale steps (`step_k`, `B^k`) belong
  here, and they act through the frames: zoom rebases the unit (C-SC3); refocus changes
  exposure and frames.
- **In-place LoD** acts within a specific SNode sub-tree: any exposure change (expand
  or collapse) at an **unchanged** frame and `step_k`. Expanding a secondary
  compression attachment is its defining example (Patrick's case): it reveals deeper
  component depth locally, at ratio 1 (C-SC6 to C-SC8). Collapsing or expanding an
  ordinary composite at a fixed frame (C-ID6) is also in-place LoD. The meta-structure
  LoD and the focus of the rest of the composition are unchanged.

Viewing different layers of component depth from a single macro perspective is
in-place LoD inside a sub-tree while meta-structure LoD holds. An in-place LoD change
never inserts a frame or alters a `step_k`, whether the composite is a secondary
attachment or an ordinary one; a meta-structure LoD change always acts
through the frames.

## 6. Collapse and expand publication

Goal: an aggregate and its exposed parts never both contribute to mass, fields or
sibling participation; repeated occurrences stay distinct; reused definitions never
share mutable particle state.

### 6.1 Exposure states

Each composite occurrence is in exactly one state in any published view:

- **Exposed:** its parts hold slots and act as ordinary particles; the composite
  holds no slot of its own.
- **Collapsed:** one aggregate holds a slot (a marble-shaped Markov blanket whose
  boundary inputs and outputs are the interaction points); its descendants hold no
  slots.
- **Not exposed:** neither holds a slot and nothing contributes.

### 6.2 Rules

- **C-PUB1. Exactly one contributor per occurrence.** Mass reaches the universal sum
  (`field.cpp:155-158`), group centroids (`field.cpp:616-625`) and sibling groups
  from the occurrence's *current-state* slots only. Never from both the aggregate and
  its parts.
- **C-PUB2. Atomic at the tick boundary.** Claims, releases and edge/bond changes for
  a state change are staged on the host and uploaded together (`Harness::upload`,
  `field.h:122`), so no tick reads a half-published state. State changes take effect
  between ticks only.
- **C-PUB3. Release-then-claim in one staging.** For a collapse: parts' slots are
  released (section 4) and the aggregate claimed; for an expand: the aggregate is
  released and the parts claimed. Both appear in the same staged state, with edges
  and bonds of released slots removed and new ones created for claimed slots.
- **C-PUB4. Sibling participation counts live occurrences.** The automatic
  same-token sibling field (`WORKING-SET-AND-LEDGER.md:112-115`) includes each
  *exposed* occurrence once, by its own slot. A collapsed occurrence's hidden
  descendants are not members. A repeated occurrence is a separate member.
- **C-PUB5. Repeated occurrences are distinct contributions.** Each occurrence has its
  own slot, `occurrence_key`, mass contribution and edges, even when it expands the
  same definition. Ordered parent occurrences remain distinct and repeated parent
  contributions remain separate edges.
- **C-PUB6. Reuse is of the definition, not the state.** A deduplicated definition is
  referenced from the warm definition (membership and order; where any gap or
  position information lives is #116's, section 8.3). Materializing an
  occurrence creates new slots with fresh state. No two occurrences address the same
  slot, and no mutable particle state is aliased between them. This matches Taichi:
  there is no mechanism to share one physical child subtree among parent cells
  (`taichi-snode-mechanics.md:101-128`).
- **C-PUB7. Collapse then expand is deterministic.** Re-expanding reconstructs the
  same set of `occurrence_key`s and order from the definition, with valid
  references and no leftover slot. Dynamic state of re-expanded parts is not claimed
  to equal their pre-collapse state: that needs an explicit aggregation contract
  (plan, Verification matrix), which is open (decision D7). Mass accounting is
  structural, not derived from volume or radius.
- **C-PUB8. Edge, bond and group capacity.** The edge, bond and group counts are
  fixed at harness construction (`field.h:87-91`) and the arrays are created from them
  (`field.h:157-164`). Publication that changes any of these counts therefore cannot
  resize the arrays of a built `Harness`: it must either construct a new `Harness`
  (a harness-construction change, with a download/upload of surviving state) or fit
  within pre-sized capacity padded with inert entries. This is decision D4; either
  must satisfy C-PUB1 to C-PUB3.
- **C-PUB9. Groups.** Groups (the composite centroids, `GroupField`, `field.h:32-38`)
  are separate arrays from particles, and the built harness has no free state for
  them. The contract requires that a group whose composite is not exposed has no edge
  naming it, so that its accumulated mass is zero (accumulators are cleared each tick,
  `field.cpp:137-143`; the publish divisor is floored, `field.cpp:645-656`). A group is
  claimed by pointing edges at it and released by removing them. Group slots are
  assigned and freed on the host in the same staging as particles (C-PUB2). The
  contract does not decide whether a collapsed aggregate is represented by a group, by
  a pooled particle (`OPERATIONAL-PLAN.md` section 3.11 describes centroid virtual
  particles drawn from the pool), or by both; that is decision D14. Whichever is
  chosen must satisfy C-PUB1.

## 7. Pool claim, release and recomposition

Claim and release are declaration into the preallocated pool (section 4). The numbered
sequence for one published view change (refocus, zoom, expand or collapse):

1. **Prepare the target view** on the warm side: frames with `step_k`, the occurrence
   set with `occurrence_key`, `token`, `frame`, `parent_occurrence`, `ordinal` and
   exposure states. Keys come from the composition, so a refocus recomputes frames and
   exposure but not keys. Cold records are read-only.
2. **Diff by `occurrence_key`:** survive (same key, still exposed), release (exposed
   before, not exposed or changed state now), claim (newly exposed).
3. **Check capacity:** claims must not exceed the free set. Exceeding the budget is a
   reportable condition to the harness, not a reason to allocate beyond the pool. What
   the harness then does is not specified here.
4. **Stage on the host:** survivors keep slots, with positions converted for any
   frame change; released slots are zeroed to `0x`; claimed slots receive token, position,
   velocity and the structural/component mass of the token they hold (aggregate mass is not set here: it awaits the aggregation
   contract, D7); edges and bonds, and group assignments (C-PUB9), are rebuilt for the
   new exposure.
5. **Validate** the staged state (range check of endpoints as in
   `field.cpp:665-681`, the C-0X4 invariant, and C-PUB1: no occurrence has both
   aggregate and part slots).
6. **Upload at a tick boundary** (C-PUB2), update claim generations for released or reclaimed slots if
   the design in D2 uses them, and retire the old view's frame records.
7. **Taichi side:** if any SNode tree is registered or destroyed to serve storage,
   that is substrate bookkeeping. It spends the monotonic node-id budget
   (`taichi-snode-mechanics.md:157-184`) and may recycle tree ids, but it changes no
   `particle_id`, `token_id` or edge endpoint (C-ID3).

Rules that apply throughout:

- **C-POOL1.** No allocation happens on claim or release of a pool slot. The particle
  pool is allocated once. This does not cover edge, bond and group arrays if D4 is
  resolved by constructing a new `Harness`; that is a harness-construction change and
  is stated as such.
- **C-POOL2.** A claimed slot is never reinterpreted as another occurrence without an
  intervening release (C-ID6).
- **C-POOL3.** Release never changes cold data. Nothing here writes to, resets, drops
  or truncates a database (AGENTS.md work discipline).
- **C-POOL4.** A refocus that exposes different primary relationships over the same
  cold objects reuses the same `token_id`s and, through unchanged view-independent
  `occurrence_key`s (C-ID4), the same survivors; only view tables, frames and the
  affected slots change.

## 8. In-place nesting example and structural fixture design

### 8.0 The legal document is an example, not a built artifact

A legal document whose text references deduplicated boilerplate, itself nesting
further boilerplate, is the **defining example** of in-place nesting. It is not
something this work builds, and no legal text is ingested or sourced.

The predicate (Patrick, 2026-10-10): expanding a compressed component in place reveals
deeper **component depth** while the **macro perspective stays fixed**. The enclosing
document keeps its focus and its scale; it does not slide out of focus or rescale.
In Patrick's words: "if you were looking at a doc and needed to expand a compressed
piece of boilerplate, you would not want the rest of the doc to slide out of focus."
One may view different layers of component depth from the single macro perspective.

This is **in-place LoD** (section 5.4): ratio-1 secondary-compression expansion within
one SNode sub-tree (C-SC6 to C-SC8), with the **meta-structure LoD** held fixed. No
frame is inserted and no `step_k` changes. The example illustrates in-place LoD, not a
meta-structure LoD change. It is **distinct from zoom and refocus**, which are
meta-structure LoD operations acting through the frames: zoom rebases the unit and
changes `step_k` (C-SC3); refocus changes exposure and frames. The plan's
Verification-matrix observation, that surrounding scale and coordinates are not rebased
solely by expansion, is kept as an observation the structural fixtures below must
satisfy.

### 8.0.1 What would be built

**Design only.** Nothing here is implemented, no data is inserted, and nothing is
claimed to pass. The Phase 1 fixtures are structural cases constructed in C++ test
memory (host side, no database writes) over tokens already seeded in the encoding
floor, read-only. They cover nested-dedupe expansion at a fixed macro frame; the
1, B, B^2 hierarchy; the equivalence case; refocus; and collapse/expand. The names
`D1`, `D2` and `Def_A` to `Def_C` below are structural roles in those fixtures, standing
in for the example's document and boilerplate; they are not legal text.

The seeded floor is only two levels deep (16 hex atoms and 256 couplets,
`kernels/database/HANDOFF.md:5-8`). The `Def_A` to `Def_C` nesting (depth 0 to 3) and
the `D1`/`D2` composites therefore cannot exist cold without database writes, which
this work does not make. They are **warm, in-memory study composition**, so the
`occurrence_key` anchor for them is a warm-composition anchor (C-ID4, D12). The
cold-chain part of a key (token and ordinal along `token_parent` rows) is exercised
only to couplet depth against the seeded floor.

### 8.1 Structure

Primary boundaries (explicit, model roles):

- `Lf` finest conceptual level.
- `Lm` the next level, composed of `Lf` elements.
- `Lc` the document-set level, composed of `Lm` content.
- Optional equivalence `Lf'` that names `Lf` differently (the UTF-endpoint to
  character case), declared with no scale step.

Secondary definitions (compression only, **no scale boundaries**):

- `Def_C`: a literal run.
- `Def_B`: a literal run and a reference to `Def_C`.
- `Def_A`: a literal run and two references (to `Def_B` and, directly, to `Def_C`).

Documents:

- `D1`: references `Def_A` twice and `Def_B` once directly, with its own literal runs.
- `D2`: references `Def_A` once and has its own text.

Partitions of the same expanded `D1` content:

- `P_nested`: as above (dedupe nesting depth up to 3).
- `P_flat`: no dedupe; the same content written inline.
- `P_shifted`: definition boundaries moved so `Def_B`-like runs cut at different
  points.

### 8.2 Views

- **View 1:** `Lm` is the finest visible frame (`step_k = 0`); `D1` content exposed;
  `D2` collapsed to an aggregate; `Lc` at `step_k = 1`.
- **View 2 (zoom in):** `Lf` becomes the finest frame (`step_k = 0`); `Lm` moves to 1,
  `Lc` to 2 (a 1, B, B^2 hierarchy).
- **View 3 (refocus):** the study is refocused on an occurrence of `Def_A` in `D2`
  instead of `D1`, over the same composed objects. Occurrence keys are anchored in the
  composition (C-ID4), not the view, so every occurrence of a shared object keeps its key.

### 8.3 Observations the structural fixtures must support

Each maps to a row of the plan's [Verification matrix](model-realignment-plan.md).
Tolerances are declared per test at implementation time and are not fixed here.

| Plan row | Fixture observation |
|---|---|
| Legal document with nested deduplicated boilerplate (the in-place nesting example, 8.0) | In View 1, expanding at a fixed macro perspective, every occurrence expanded from `Def_A`, `Def_B`, `Def_C` carries the same `frame` and so the same `step_k` as the surrounding `D1` content, at nesting depth 0, 1, 2 and 3. Expansion alone leaves every `step_k` and all surrounding coordinates unchanged: it is not a refocus. |
| Same literal, different compression partitions | `P_nested`, `P_flat` and `P_shifted` expand to the same ordered sequence of exposed occurrences with the same order. Total mass and the universal sum are identical across partitions; no scale or mass inflation tracks nesting depth. |
| Repeated boilerplate in one/two documents | `Def_A` appears twice in `D1` and once in `D2`: three distinct `occurrence_key`s, three disjoint slot sets, three independent edge sets. Changing the state of one occurrence leaves the other two unchanged. No slot is shared. |
| Mixed primary and secondary hierarchy | Only the `Lf/Lm/Lc` boundaries change `step_k`. Inserting or removing a secondary level (for example wrapping `Def_C` in another dedupe layer) changes no `step_k`. |
| UTF endpoint / character equivalence | The `Lf'` equivalence yields `Δk = 0` and no distance step; the naming change is retained in the frame record. |
| Conceptual zoom and two study focuses | View 1 to View 2 shows `step_k` values 0,1 become 1,2 on `Lm`, `Lc` (plus 0 for the new `Lf`) with identity, ordinal and `token_id` retained and survivors keeping slots. View 1 to View 3 retains the `occurrence_key` and slot of every shared object still exposed (for example the `D2` occurrence itself and its still-exposed parts), while exposure and `step_k` differ and slots of no-longer-exposed occurrences are released. State is compared in a canonical common frame within declared tolerances. |
| Collapse then expand | Collapse a `Def_A` occurrence and re-expand it: exactly one of aggregate or parts ever holds slots in any published state (C-PUB1); the same `occurrence_key`s are rebuilt in the same order; all edge and bond endpoints are valid. No claim of identical force or dynamics. |
| Whitespace | **Deferred to #116.** The contract defines no field for gaps (ordinal is not a spatial coordinate, #115). Gap preservation across compression and expansion, and any optional separator, are #116's observations and are not asserted here. |
| CPU then available CUDA | The same observations are checked on CPU and, where a device exists, CUDA; unavailable device coverage is recorded as unavailable. |

Rows for size examples and for small versus larger connected models belong to the
extent (#120) and calibration phases and are not covered by these fixtures.

### 8.4 Additional checks specific to this contract

- Varying dedupe nesting depth changes no scale-dependent output (C-SC7).
- A forced re-run of the publication with no state change produces no slot churn.
- Releasing all slots of a view returns the pool to all-`0x`, with the C-0X4 invariant
  holding throughout.
- Free-slot numerical behaviour (section 4.3) is a precondition, not an observation of
  these fixtures.

## Open decisions for review

Each needs Patrick unless a later phase's evidence settles it. Recommendation first,
tradeoffs after.

| # | Decision | Recommendation | Tradeoffs |
|---|---|---|---|
| D1 | Explicit versus derived resolution of identity, scale and position (plan step 1). | Hybrid in section 2.3: explicit token, slot, parent, ordinal and a frame reference; scale held once per frame. | Derivation by walking is smaller but its cost tracks storage depth, the quantity that must not matter. Explicit costs a few integers per occurrence. |
| D2 | Is `particle_id` the pool slot, or an id with an indirection table to slot; and are claim generations used and where do they live? | Slot, with a bridge-side claim generation if stale-reference detection is wanted (C-ID1). | Indirection allows slot compaction or relocation but adds a lookup on every reference and a remap on every change. The pool is fixed, so relocation is not expected. |
| D3 | How free (`0x`) slots are made numerically inert (the `field.cpp:463` divide, whole-pool loops). | A branchless arithmetic gate consistent with the force kernel, plus a test, in the first behavioural PR that holds free slots. | Alternative is an active-range or compaction scheme, which changes loop bounds and may conflict with the planned wake model (`ACTIVE-FIELD-MODEL.md:145-174`). |
| D4 | Edge, bond and group capacity at publication: construct a new `Harness` (counts are fixed at construction, `field.h:87-91`) or pad pre-sized arrays with inert entries. | New `Harness` at refocus/zoom; consider padding only if hot collapse/expand cycling shows the cost. | Rebuild is simple and low-frequency but is a harness-construction change with state download/upload, and it breaks "allocated once" for those arrays; padding avoids reallocation but carries inert entries through every pass. |
| D5 | Frame-to-frame factor: common scalar through view conversion, or relationship-kind multiples, and where any multiple lives (frame, object pair, warm SNode definition). | Leave open (#115). No placement is fixed; test 64 first. | Fixing either now would pre-empt the calibration evidence #115 asks for. |
| D6 | Exact encoding of the view-independent `occurrence_key` and its compact handle. | Cold token_id plus `(parent occurrence, ordinal)` along the cold composition to the anchor composite (C-ID4), with a bridge-local compact handle derived from that path. | Stable across focus, zoom, exposure and dedupe; a long path needs the handle to keep per-occurrence memory depth-independent. |
| D7 | Aggregation contract for a collapsed occurrence (its mass and boundary inputs/outputs). | Defer to the phase that implements collapse; this contract requires only structural mass accounting, once. | Needed before any claim that collapsed and exposed dynamics agree. |
| D8 | Whether sparse SNodes stage optional LoD detail at all. | Not needed for this contract; decide in Phase 4 with a measurement of activation allocation. | Sparsity helps partial pulls but activation allocates (`node_pointer.h:56`) and is not the pool claim. |
| D9 | Hot form and width of the token reference. | Defer; keep `token` an opaque reference in the contract. | Cold `token_id` is an address key (`docs/address-encoding-transition.md`); a compact hot handle may be wanted, which must not become a new identity. |
| D10 | (Removed 2026-10-10, resolved by Patrick.) The legal document is an illustrative example, not ingested data, and the proposed fixtures would use seeded encoding-floor tokens read-only. | n/a | n/a |
| D11 | Cache-key and schema for prepared views. | Keep open until this contract is reviewed (plan step 4). | None until chosen. |
| D12 | Anchor composite for `occurrence_key`: which cold composite ends the chain, and what happens to keys when the composition itself changes (objects added, removed or re-parented) or a token has several cold parents. | The outermost cold composite(s) of the warm composition (here the document composites, which exist only as warm, in-memory composition in the fixtures), fixed for a composition revision; keys are re-derived only when that revision changes. | Keeps keys stable under focus, zoom and exposure; a composition change legitimately changes keys, so survival across such a change needs its own rule. |
| D13 | Where the live `token_id` association lives: bridge-side table, or in the slot (`OPERATIONAL-PLAN.md` 3.11 versus `ACTIVE-FIELD-MODEL.md:133-139`). | Bridge-side table indexed by slot; device arrays unchanged. | A table avoids changing payload layout and staging; in-slot makes the pool self-describing and a free slot visible on device but changes the payload and its readers/writers together. |
| D14 | How a collapsed aggregate and a composite centroid are represented: group, pooled particle, or both, and how groups are claimed and freed. | Decide with the aggregation contract (D7). | `OPERATIONAL-PLAN.md` 3.11 draws centroid virtual particles from the pool; the built harness holds centroids in a separate fixed group array. |

## Adjacent observations

Noted only; none are acted on here.

- `engine/src/field/field.h:44-47` still describes edge offsets and off-centre pull,
  which the intended model withdraws (#118).
- The field arrays carry no token mapping and no sibling-field generation
  (`ACTIVE-FIELD-MODEL.md:133-139`); the recommended bridge-side token table (C-ID2, pending D13) is where that
  gap would be bridged.
- `field.cpp:155-158` accumulates universal mass over the whole pool each tick,
  independent of occupancy; relevant to the pool budget measurement already planned.
- `engine/tests/field_test.cpp` was not audited line by line for mass-zero cases.
- Open PR #111 findings are untouched and unrelated to this contract.
- `docs/README.md` indexes system-map documents; a link to this contract could be
  added after review.
