# Prepared particle space and live representation

Status: **proposed for review, 2026-10-21**. This plan addresses
[#156](https://github.com/Human-Cognome-Project/human-cognome-project/issues/156)
and Patrick's subsequent `0x` and computational-cost clarifications. The
correction is required and unbuilt. This PR changes documentation only.

Accepting this plan approves its scope and native-first investigation sequence.
It does **not** approve the bootstrap/focus-establishment, initial-velocity,
batch focus-update, tangency, tolerance or tie conventions proposed in section 4.
Native provisioning and define/release investigation can proceed independently.
Production insertion must use explicitly reviewed conventions before its
implementation is accepted; that review may be a targeted design update or
part of the implementing PR.

## The space is preconfigured; work declares its contents

Unoccupied space throughout the working expanse consists of potential `0x`
particles with **no mass, presence or fields**. A live layout declares objects
into that prepared space and releases locations back to `0x`. The construct
persists while its representation changes.

The point of preparing a large particle space is to make its ordinary use a
small assignment. A potential entry should require only nominal declaration
storage, targeting one or a few bytes per entry, with actual native overhead
measured. It must not require a populated particle payload, object identity
record, physical coordinate or relationship listing for every empty location.
Existing payload and related working storage are prepared under their own
finite budgets and assigned as needed. A large virtual span does not imply
that its entire expanse carries a full numerical particle record.

`0x` describes empty potential here; it allocates no cold token or linguistic
address. Slot/index topology is independent of physical coordinates and defines
no spatial lattice. “Virtual mass” means capacity available for representation;
it contributes no mass to the model.

| Distinction | Required behavior |
|---|---|
| Potential `0x` space | Lightweight prepared declarations. Empty entries contribute no object mass, occupied space, membership, contacts or numerical particle work. |
| Present representation | Defined objects with identities, extents, mass and relationships, using assigned prepared working records. Sleeping objects remain present and occupied. |
| Calculation activity | The existing independent binary awake/sleeping gate on present elements, consumed according to particle/centroid phase. Sleeping does not release representation. |

These distinguish capacity, contents and work; they introduce no new multistate
scheduler. Claims/redeclarations and releases are ordinary changes inside
prepared space. Backing allocation and compilation belong to preparation or
an explicitly separate reservation change.

## Patrick's threefold computational rule

| Action | Cost model and implementation direction |
|---|---|
| Declaration | Touches one data point and is near free. Prefer defining/referencing or clearing an existing line item whenever that expresses the required change. |
| Calculation | Touches at least three data points plus the calculation resource. Calculate when needed, and use the loaded data for all required related work while it is hot. |
| Allocation | Adds allocation bookkeeping to the calculation that caused it. Avoid it in hot runs by preparing reusable capacity beforehand. |

This is the governing design rule. A LoD change uses definition and release
within the prepared working space. It must not expand into a new study/tree,
compilation or backing-allocation flow for each change. Keep declarations narrow;
perform necessary derived work using already hot values through the native engine
phases. Reuse current mass, position, field and relationship values for applicable
updates instead of repeatedly fetching or recomputing the same inputs. Preserve
mathematical dependencies and the accepted particle/centroid phase order.

Measure declaration, required calculation and backing allocation separately.
Calling an allocation “activation” or hiding it inside metadata work does not
satisfy the rule. Report native bookkeeping separately from declaration
assignments so hidden allocation or repeated calculation cannot be counted as
cheap declaration.

The rule guides design; measured costs determine actual behavior. Include
metadata and relationship work caused by a declaration in those measurements.
A narrow declaration assignment does not establish the cost of the complete
admission/update operation or a measured bytes-per-entry result.

## Source baseline and correction surfaces

Baseline: `main` at `5fa8a63`, including accepted #155 and #157–159.

| Surface | Existing assumption to correct |
|---|---|
| [Warm values](../kernels/database/warm/values.h), [schema](../kernels/database/warm/schema.sql), toolbox | Recipe v1 requires and persists occurrence position/velocity as starting inputs. |
| [Projection](../engine/src/assembly/letter_projection.h), active inputs | Aggregate/byte starts and local world-placement overrides are required/accepted. |
| [Study staging](../engine/src/assembly/study.cpp), [workbench](../engine/src/assembly/active_workbench.cpp) | Count-sized studies, equal-count refresh and clearing/reloading numerical state. |
| [Harness](../engine/src/field/field.h), [Activity](../engine/src/field/activity.cpp) | Fixed particle/group/edge/contact strides, adjacency, scratch and whole-publication resets. |
| [NativeSet](../engine/src/field/native_set.h), fork runtime | Pointer activation can obtain nodes; backing and traversal lists have lazy chunks. A declared address range does not prove provisioned backing. |
| [Capture](../tools/viewport/native/letter_capture.cpp) | Handwritten world starts and supplied fixture extents; backend, ownership and cleanup safeguards already exist. |

Warm remains the toolbox, hot the assembly workbench, and Taichi execution the
authority for current position, velocity and motion history on CPU or GPU.
Staging/readback copies are transport or observations, not another authority
that resets survivors. Intrinsic component-relative geometry, ordered
construction and scale remain valid inputs; world placement is a different role.

## 1. Establish native provisioning and the declaration fast path

Start at the fork's native SNode, reservation, activation, recycling and traversal
interfaces. Existing `Pointer_activate` calls `NodeManager::allocate`;
`ListManager::touch_chunk` can request directory pages and backing chunks.
Trace both ordinary CPU allocation and the CUDA reserved-memory path.

The first implementation deliverable is a measured native storage decision:

- Describe the lightweight `0x` declaration span separately from prepared
  numerical payload, field/group, edge/contact, identity/reference and scratch
  budgets. Show measured bytes per potential entry and per present object,
  including metadata, alignment and padding. Do not multiply the full current
  `Harness` payload by the entire virtual particle count.
- Check how native facilities can provision/recycle backing and traversal lists
  before operation while leaving zero objects present. Compare appropriate
  native sparse/bitmasked layouts; preserve the tested power-of-two padding
  requirement. Provisioning storage must not instantiate dummy physical objects
  or fill empty space with coordinates.
- Prepare the supported representation families, kernel bindings and actual
  first dispatches in advance. Include presence/awake traversal, adjacency,
  reductions, selected focal references, occupancy/contact work and reusable
  update scratch. A particle reserve with per-arrival field/edge allocation
  leaves the original stall in place.
- Exercise declarations and releases in previously unused blocks within the
  reservation. Native logical claims/recycling must use existing backing and
  compiled code. Record any missing native reservation hook and propose the
  smallest extension needed; do not invent a competing allocator or scheduler.
- Finish preparation with no model mass, occupied objects or memberships.
  Empty potential must be skipped by live enumeration; account separately for
  any native container/list-generation visits.

Instrument setup, first touch, ordinary declaration/update, calculation and
explicit resource-limit handling. Count backend allocations, reserved-pool
suballocations, native chunk/list growth, layout builds and compiler invocations
separately. Report host metadata allocation and launch costs too. Ordinary work
inside the prepared budgets must require no new backing allocation, layout
rebuild or compilation. Requested capacities remain finite and configurable.

## 2. Coordinate-free structural recipes and retained compatibility

Introduce recipe v2 preserving exact scoped definitions/revisions, occurrence
paths, parent order/counts, memberships, explicit conceptual roles and selected
exposure. Shared and active-only local structural requests need no world
coordinate or velocity arrays, including inactive alternatives.

Use additive, versioned warm migration and reader/writer dispatch. Retain v1
records and their historical starts for audit and old fixture reproduction.
New operational publications use v2; a labelled compatibility adapter may read
v1 structural content while ignoring legacy starts as execution authority.
Old readers must refuse unsupported v2 data cleanly. Verify transactional
migration/publication, interruption, exact readback and repeatable reruns without
mutating old immutable revisions or relabelling old evidence.

Component-local geometry and the existing supplied-layout enclosure facility
remain distinguishable from world placement. Reconcile component-derived extent
with #120; the capture's fixed 0.25/0.125 values remain fixture inputs, not a
general radius rule. Any retained arbitrary-world-layout numerical fixtures use
an explicit test-only route.

Core/Linguistics/warm connections are configuration independent of snapshot
filenames. Migration uses explicitly configured targets. No database rename,
reset/drop/truncate, new database per ordinary run or cold identity change is
part of this correction. Private inputs remain analyst-owned and active-only;
the shared manager still refuses private scopes before source lookup.

### Optional restart state

Patrick explicitly permits optional supplied positions on engine restart as a
calculation hedge/convenience. Preserve that recovery path: optional validated
execution checkpoints or restart hints are separate inputs from immutable warm
structural recipes and from legacy fixture starts. Coordinate-free structural
requests do not prohibit an owner from supplying such optional recovery data.

At an explicit restart, validate any supplied state against the owner's logical
objects, definition/representation bindings, numerical frame/units and supported
recovery contract before admitting it. The engine owns the restored state after
admission. Neither ordinary streaming nor refresh may apply restart hints to
reset running survivors. Legacy recipe starts do not automatically qualify as
a validated checkpoint; any recovery use needs its own explicit validation and
restart context. Private/local recovery data stays with its active owner and
does not enter the shared manager's structural publications.

No full checkpoint storage/transport subsystem is requested by this correction.
The implementation must preserve the optional validated input distinction and
test the supported recovery boundary without making checkpoints mandatory.

## 3. Continuous declaration and engine-owned insertion

Use existing engine synchronization boundaries between native phases/ticks to
publish coherent batches. Do not synchronize, re-project a study or build a
new allocation pipeline for each declared line item.

For a batch of genuinely new objects:

1. Validate owner, current handle, structural completeness and all required
   resource budgets before changing visible state. Include relationships and
   the temporary capacity needed by the supported operation.
2. Define newcomers in unused prepared locations, assigning prepared records
   and initializing their required values before visibility. Source occurrence
   identity is independent of its current numerical location/index.
3. Use engine-owned insertion at the nearest unoccupied space to the current
   combined centroid of selected focal fields, respecting scale-correct extents.
   Use the exposed-body occupancy set below, including sleepers and previously
   placed batch bodies. Batch focus-update ordering remains a section 4 proposal
   requiring review before production insertion is accepted.
4. Calculate required relationship, inclusive-centroid, universal-mass and
   contact/wake effects while their inputs are hot. Add contributions to existing
   fields and define new ones within prepared budgets. Caller contact lists are
   not a complete occupancy query; an empty list cannot hide an occupied body.
5. Preserve surviving position, velocity, motion history and unaffected activity.
   A coordinate rebase changes the numerical reference coherently, not the
   physical arrangement. New/affected work enters the existing binary type
   phases; empty potential never becomes numerical work.
6. Publish the batch's consistent counts, references and report generation,
   then continue ticking the same construct. Advance generations at coherent
   publication boundaries, not through a new handle ceremony per declaration.

Rework full-array staging, relationship-index updates and publication resets
on BulkCopy and CpuDirect. Keep allocated strides stable as present counts change.
Where calculation must touch all relevant contributors, traverse present objects
and reuse their values; never scan all `0x` entries to find work.

Validation/capacity failures preserve the previous committed state and generation.
If a backend failure cannot be recovered through prepared native scratch, make
execution explicitly unready instead of claiming continuity or restoring recipe
starts. Preserve owner guards and explicit stale-handle/report semantics; a
released/reused location must not let an old reference name a different object.

### Occupancy at supported LoDs

“Present” for an occupancy query means an **exposed physical body in the
committed representation**, with its actual model extent. It does not mean
every organizational or numerical record associated with that representation.

| Supported representation | Bodies included in occupancy | Records that add no extra solid obstacle |
|---|---|---|
| Aggregate exposure | Each explicitly exposed aggregate occurrence, once, at its scale-correct model radius. | Hidden component spheres used for enclosure, dormant part alternatives and equivalent naming records. |
| Parts exposure | Each explicitly exposed part occurrence (bytes in the bounded letter projection), once, at its own scale-correct model radius. | Its inactive aggregate alternative, hidden construction ancestors and any unexposed descendants. |
| Mixed conceptual levels/exposures | The union of explicitly exposed physical occurrences, all converted into the common operative frame. Distinct occurrences remain distinct even if they share a definition. | No automatic second body for a shared definition, scale binding or compression/naming attachment. |
| Centroid/organizational fields | No body from a field record alone. If an object has a separately declared physical representative, count that representative through its body record. | Sibling-definition, parent-value and membership centroids, including their participating mass; membership/construction links themselves. |
| Sleeping physical occupants | The same bodies and extents as when awake; they still occupy space. | Sleep does not create free space or remove participation. |
| Potential `0x` entries | None. | Empty storage, inactive alternatives and released records add no occupancy. |

This preserves the existing distinction between prepared exposed particles and
typed fields in [study preparation](../engine/src/assembly/study.cpp); it defines
no new force, extent or LoD-selection policy. A field's participating mass is
not an invented radius or enclosure. Occupancy coverage is independent of the
supplied contact ledger; coverage does not itself mandate all-pairs contact
forces or new structural relationships.

For a validated representation exchange, the prospective occupancy set contains
surviving exposed bodies and replacements already placed, excluding only bodies
explicitly retiring in that same exchange. The old representation remains the
externally committed state until the coherent swap. Never count an aggregate
and its mutually exclusive part alternative as simultaneous obstacles during
placement, or expose an intermediate gap/double representation in reports.

## 4. Numerical details proposed for insertion review

The nearest-free-to-focus rule is settled. The remaining conventions below are
proposals, not new placement policy or a requirement to reprove model mechanics.
Plan acceptance leaves these conventions unapproved. Native storage/define-release
investigation needs no chosen physical bootstrap layout; production insertion
acceptance requires a reviewed initialization and numerical contract first.

For newcomer radius `r`, focus `F` and current occupied centres/extents
`(c_i, R_i)`, minimize displacement from `F` subject to
`distance(x, c_i) >= r + R_i`. Propose allowing tangency and validating the final
stored position against outward-rounded extents. State a coordinate/ULP-derived
numerical bound; do not add a universal physical gap or change contact/force laws.
Use `F` directly when free. Otherwise the search must justify nearest admissible
placement within that bound, including overlapping exclusion regions. A small
independent sphere-boundary/intersection reference can check native results;
first-found free space or a prescribed lattice is insufficient.

Propose the following explicit conventions for review:

- A candidate numerical gauge is local `(0,0,0)` with a separate reference.
  Choosing a gauge does not position the first bodies or establish their focus.
  The first focal participants themselves have no positions yet: deriving a
  centroid from them is therefore not a complete initialization rule. A reviewed
  engine bootstrap sequence must resolve that dependency and establish the first
  positions/focus before ordinary insertion can use it. Initial relational
  instantiation remains unresolved here. Do not fill the gap with supplied
  layouts or silently treat the gauge as an approved placement algorithm.
- An unavailable selected focus in an existing construct is an explicit
  operation failure. Preserve explicit no-selection/legacy behavior separately;
  do not choose a replacement focus silently.
- Insert batch members in stable occurrence/ordinal order, updating prospective
  occupancy as bodies are placed. Updating prospective focus after each placed
  participant is a proposal, not an approved ordering. Propose zero velocity
  for genuinely new arrivals; surviving velocity remains current engine state,
  and optional restart-state admission follows its separate recovery contract.
- Equal-distance candidates use a deterministic numerical direction order,
  independent of address and thread scheduling. Document the precision/tie rule
  and results for degenerate/zero-extent cases. Report an unsupported or
  unattainable numerical result explicitly rather than inventing a radius or
  overlapping placeholder.

Keep focus at admission, the offset needed to find unoccupied space, and later
field-driven displacement distinct in observation. Preserve local-coordinate
precision and the separate double reference.

## 5. LoD is definition and release in the prepared space

A perspective/layout change redeclares the required representation and releases
unneeded line items back to `0x`. Reuse prepared definitions, payload records,
field/edge capacity and compiled operation families. No new tree/study or
allocation flow is required for each ordinary LoD shift.

Distinguish re-expressing an existing construct from inserting an unrelated new
arrival. Preserve surviving state; calculate only the necessary scale/extent,
centroid and participation consequences from current engine values and prepared
definitions. Do not reset everything or apply a fresh-start arrangement to the
whole construct. Any newly required numerical state must follow an explicit
engine instantiation rule; the pool itself supplies no geometry or layout policy.

Demonstrate a bounded explicit representation exchange and release/reuse using
prepared letter/byte definitions. Retain underlying identities and ordered
construction. Ensure aggregate/parts representation does not double count mass
or participation, and that selected focal bindings remain coherent. Primary
conceptual steps retain their conversion; compression/naming attachments remain
1:1. Detailed perspective selection, slopes and general recomposition are not
inferred from this storage correction.

Release removes the retired item from presence, occupancy, mass, fields/contact
participation and awake traversal before reuse. Return its working records via
native recycling. Logical `0x` does not require clearing every backing byte on
every release: old payload must be inaccessible as a represented object, and
a new definition initializes what it needs before visibility. Sleeping never
releases an item.

At a true declaration/payload/field/edge/contact/scratch limit, propose an
explicit `CapacityExhausted` result identifying the budget, leaving the current
construct intact. Crossing a native block inside capacity is ordinary work.
An explicit reservation increase is a separately measured resource operation;
the first correction can demonstrate defined exhaustion rather than promise
an unimplemented expansion path.

## Delivery and acceptance

There are separate review/acceptance boundaries. Native provisioning and
define/release investigation can proceed without resolving physical bootstrap.
Accepting this planning PR does not select any section 4 convention. Before
production insertion is accepted, explicitly review its bootstrap/focus sequence,
initial motion, batch focus-update order and numerical/tie contract. The eventual
acceptance cases below test that reviewed contract; listing them here does not
approve an otherwise unresolved convention or authorize supplied-layout fallback.

1. **Native foundation:** measured lightweight declarations, prepared backing,
   supported layout families and a narrow define/release path. Exercise unused
   blocks and release/reuse; prove where backing and compilation occur. Storage
   probes may leave all entries without physical presence and require no
   production placement convention.
2. **Structural-to-live correction:** retained-v1 compatibility, v2 requests and
   coherent insertion/update together, using the explicitly reviewed insertion
   conventions. Tick a construct, stream repeated batches without required
   structural starts, and continue ticking with survivors intact. Continuous
   insertion is required in this first working path. Separately test optional
   validated restart state and its rejection outside the supported recovery path;
   no full checkpoint subsystem is an acceptance prerequisite.
3. **Representation changes:** demonstrate explicit LoD redeclaration/release,
   reuse and true resource exhaustion within the same prepared construct, with
   correct mass, fields, focal bindings, state and native activity.
4. **Real capture and reconciliation:** re-run configured source → warm → active
   → report to demonstrate ordinary structural inputs need no world starts.
   Label any optional restart-hint recovery capture separately; its allowed
   state does not become a required recipe input. Retain #158 backend verification
   and #159 read-only live-capture cleanup/reference handling. Update A–D/pre-D,
   projection/object/field/workbench and warm guidance together as historical
   limits versus corrected behavior. Keep old evidence labelled as such.

Report reserved virtual entries/declaration bytes, prepared backing bytes,
present objects/fields, live relationships/contacts and awake/evaluated work
separately. Measure definition/release and necessary calculations alongside
setup, first touch, backend allocations, native growth and compilation. The
ordinary prepared-capacity path must add no backing, rebuild no layout and
compile no code. No infinite-capacity or universal latency claim is proposed.

Acceptance must include free/occupied focus, empty-focus bootstrap, variable
extents, sleeping occupants, multiple arrivals, ties and overlapping exclusions;
aggregate/parts/mixed occupancy without extra centroid or hidden-component bodies;
old/new fields and contacts; carried motion and unaffected sleepers; previously
unused blocks; repeated LoD changes and released-slot reuse; exhausted supporting
budgets, invalid input, stale handles and backend failure. Verify exact identities,
contributions, current centroids and coherent reports without mixed generations.
Check transactional migration/readback, unchanged cold stores and private-manager
exclusion. Use independent numerical expectations and allocation/compiler counters.

CPU implementation evidence and Patrick's subsequent local GPU/browser run are
separate handoff stages; previous GTX 1070 and supplied-layout results do not
validate this correction. The viewer continues observing current positions and
receives no placement controls. Version any snapshot-contract change explicitly;
sidecar evidence can carry capacity/insertion measurements without expanding UI.

Review is requested on the storage/provisioning sequence, retained compatibility,
coherent live updates and proposed bootstrap/velocity/tangency/tie/exhaustion
conventions. Patrick's `0x` model, threefold cost rule, continuous insertion and
LoD definition/release direction are governing requirements, not open alternatives.
