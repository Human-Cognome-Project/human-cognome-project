# Combined prepared-capacity foundation

Status: **accepted in #163, 2026-10-21; implementation under #162**.
This is the next bounded segment of
[#156](https://github.com/Human-Cognome-Project/human-cognome-project/issues/156),
following [the accepted overall plan](pooled-space-live-insertion-plan.md) and
[#161's accepted storage investigation](../engine/docs/PREPARED-SPACE.md).
Baseline: `main` at `6b2fac211633dd39fdebc05eb7fa1a04a205a116`.
The accepted PR changed documents only. The
[first mapping/adapter implementation](../engine/docs/RECORD-MAPPING.md) is
accepted and merged in #164 at `41310a5`. The
[next backing/binding section](prepared-execution-backing-plan.md) is drafted
for review; combined prepared field execution is not implemented.

[The final #161 review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/161#issuecomment-6322166930)
accepts dense declarations with budgeted native presence records as the candidate
and directs the next work toward combined payload/field/edge/contact/reference/
scratch budgets and supported kernel bindings. Its four symbolic bindings do
not establish general record mapping, realistic working sets or live insertion.

## Deliverable and boundary

Build one prepared native execution whose potential declaration span is much
larger than its numerical working-record budget. Define, change and release
records and relationships within that reservation, run the existing field
kernels between batches, and preserve surviving numerical state. Preparation,
ordinary updates and genuine budget exhaustion must have separate evidence.

This segment establishes the execution/storage operation beneath future logical
admission. It uses explicitly labelled numerical fixtures to exercise real
kernels; it does not implement production placement. Fixture starts enter only
through a test adapter under `engine/tests`, never as new required structural
inputs or a production fallback. No warm migration, database population or
new operational coordinate/velocity requirement belongs to this segment.

The supported numerical families are the current exposed particle, typed field,
participation and ledgered-contact forms used by aggregate, parts and mixed
letter fixtures. Binding and exercising these families does not implement a new
LoD-selection policy or a general representation transformation.

Completion here leaves #156 open for coordinate-free structural publication,
production identity/owner integration, engine-owned insertion/occupancy,
continuous source-to-report operation, supported LoD exchanges and local GPU
qualification. Section 4 of the overall plan still requires explicit review of
bootstrap/focus establishment, initial motion, batch focus ordering and numerical
conventions. Accepting this segment selects none of those conventions, nor the
exploratory #115 conceptual-span/coupling idea.

## Current code that must change

| Surface | Present behavior | Required foundation correction |
|---|---|---|
| [Harness storage and accessors](../engine/src/field/field.h), [constructor/bindings](../engine/src/field/field.cpp) | Particle/group/edge/contact counts also determine SoA strides, array shapes and host-vector lengths. | Fixed reservation strides and stable buffers; separate live counts, presence and available capacity. Preserve two-axis arrays and widened offset arithmetic. |
| [Activity construction](../engine/src/field/activity.cpp), [NativeSet](../engine/src/field/native_set.h) | Native masks, numerical flags/deltas, selected-field arrays and adjacency are sized from initial counts. | Prepare all supported budgets and native backing together. Retain one particle awake set and one centroid awake set. |
| `Activity::index` | Builds fresh host vectors and compact CSR indexes around initial counts. | Reuse reserved index/scratch storage and preserve stable structural reduction order across record reuse. |
| `Harness::publish`, `Activity::reset` | Full upload/index/reset/reseed; group mass and universal mass are built at publication. | A distinct coherent numerical-update boundary that changes affected records without replaying survivor starts or clearing their history. |
| `Harness::bind` and launch wrappers | Construct shape/dimension vectors and launch contexts repeatedly. | Audit and measure host metadata/launch allocation; reuse supported bindings/context storage instead of hiding it behind stable pool counters. |
| [FixedStudy staging](../engine/src/assembly/study.cpp) | Equality guards, whole-payload clearing and supplied-state reload. | Keep its historical fixture behavior explicit; foundation tests use the new internal boundary rather than treating refresh as insertion. |
| [ActiveSession refresh/report](../engine/src/assembly/active_workbench.cpp) | Generation replacement, FixedStudy republication and copied report context. | Preserve current bounded behavior until the later operational integration; expose coherent native foundation inspection without claiming ActiveSession has gained insertion. |

Keep runtime/binding plumbing in `engine_support` and numerical storage/tick work
in `field_core`. Refactor the existing Harness/Activity kernels rather than
building another physics implementation. The legacy fixed-count constructor may
adapt to capacity equal to initial counts; it retains its labelled fixture limits.

## 1. One finite reservation, several distinct budgets

Prepare a checked reservation description before building layouts or kernels.
Requested counts are logical limits; native power-of-two padding and actual
backing are reported separately. A budget cannot be inferred from another one.

| Budget | Reserved contents | What consumes it |
|---|---|---|
| Potential declarations `V` | Nominal byte declarations, with native layout overhead measured. | Addressable potential locations. Zero contributes no mass, body, field or work. |
| Numerical particle records `P` | Current particle SoA, determiners, motion history/deltas, presence and independent awake selection. | Present numerical bodies, including sleepers. Full numerical payload is bounded by `P`, not multiplied by `V`. |
| Field/group records `G` | Typed fields, inclusive mass/centroids, retained reduction residue/history, presence and centroid activity. | Defined field records; a field alone adds no solid body. |
| Participation `E` and contacts `C` | Exact endpoint bindings, shares/order and live ledger entries. | Live contributions and supplied body-contact pairs, with both contact directions indexed. |
| References `R` and selected focus | Declaration-to-record associations, reverse bindings, record reuse validation and focal-field references. | Bound live records/references; focus capacity is explicit and bounded by field capacity. |
| Update/input scratch | Bounded staged batches, validation/results, retired-record handling and coherent publication scratch. | Peak supported update demand, including temporary overlap before retired records are reusable. |
| Adjacency/reduction/inspection scratch | Particle/field offsets, both participation orders, both contact orders, counts/cursors, reductions and native inspection output. | Derived worst-case demand from `P/G/E/C`, batch limits and any committed/prospective index copies. |

Validate shape products, padding, index ranges and byte arithmetic before
allocation. Include reference/index and scratch amplification in the budget
formula. Preserve exact source participation/ordinal information; an index
rebuild must not merge distinct occurrences or count a contribution twice.
Do not silently add a full-width pointer, identity or full numerical record per
potential location to solve mapping. Measure all side-index overhead against
both potential capacity and working-record capacity.

The numerical boundary takes bounded internal references. Variable-length source
strings/path decoding and the later structural-request adapter are outside this
segment's allocation claim. Their cost must remain visible in eventual full
admission evidence. No cold address or identity schema changes are proposed.

### Native record mapping and reuse first

Replace #161's four hard-coded associations with arbitrary valid potential
addresses and reusable numerical records within the configured budgets. Require
forward/reverse consistency and explicit stale-reference rejection after reuse.
Physical record order, occurrence identity and potential address remain distinct.
References are execution-scoped: another prepared context's reference is invalid
even if its record index/reuse discriminator matches. This is bounded execution
safety, not adoption of held #123's general identity proposal.

Start with the fork's native pointer/dynamic SNodes, bitmasks, `NodeManager`
allocation/recycling and compiler GC. Prove how a claimed native record binds
the existing numerical arrays without per-arrival backing. Native pointer
recycling does not itself expose a general dense-array slot allocator; dynamic
append does not by itself prove individual release/reuse. Do not assume either
solves that binding before demonstrating it.

The first implementation commit must state the mapping/reuse choice, metadata
footprint and costs. If a native reservation/index-binding hook is missing,
include the smallest adapter to the existing native mechanism as a separately
reviewable change with a focused proof. A competing allocator, custom free queue
or scheduling system is outside this plan. Reusing a record must invalidate its
old reference; a reuse discriminator serves reference safety, not activity epochs.

## 2. Prepare actual storage and kernel families

Build native layouts, host/device buffers, scratch and the supported bindings
once per reservation. Precompile and dispatch the operation families for
definition/release, reference validation, presence/awake traversal, index updates,
mass/centroid work, force/contact/integration, inspection and failure results.
SNode-specific bindings keep the existing cache-identity safeguards.

Provision sparse backing and worst-case traversal-list capacity through native
facilities before ordinary use. Include first use of every reserved block and
index/scratch high-water mark. Setup must finish with no physical objects,
memberships, contacts or model mass; native storage provisioning does not create
dummy geometry. Use zero-work dispatches and storage operations, then measure
the first legal nonempty fixture dispatch separately to detect remaining laziness.

For BulkCopy, keep device arrays and host staging addresses/shapes stable and
apply bounded staged changes to current device state. For CpuDirect, reserve
host storage fully and keep borrowed pointers valid through synchronization.
Neither path may download and republish all survivor state as an update method.
Current mutable numerical state stays under execution ownership.

Kernel shapes/SoA strides use capacity; actual work uses native presence/awake
traversal or explicit compact live relationship lists. A hole is not an item,
and a live count is not necessarily the highest occupied slot plus one. Initial
seed/all-present operations must also exclude unused records and padded indices.
Never scan `V` to discover numerical work.

## 3. Coherent changes between ticks

Support batches containing numerical-fixture definitions, changes to current
participation/contact ledgers, and explicit release of nonselected records.
Use a synchronized engine boundary between complete ticks; arbitrary mutation
while a kernel is consuming the same state is not required.

1. Validate references, supported operation shapes and every affected budget,
   including prospective relationships and peak temporary storage, before any
   committed state is changed. Refuse duplicate/conflicting definitions and
   dangling edges/contact endpoints. A selected field cannot be retired without
   a supported valid replacement, and selected fields must retain valid
   participating mass. Unsupported focus changes fail before mutation. This
   segment defines no new focus-selection policy or bootstrap rule.
2. Claim prepared records and populate bounded prospective data/index storage.
   Keep the current committed view authoritative until references, counts and
   relationships agree. Do not reuse retiring storage while it is still needed
   by a live kernel, validation or the supported commit procedure.
3. Update necessary mass, affected inclusive centroids, adjacency and contact
   bindings from current numerical values. Include sleeping contributors. State
   the treatment of affected reduction residue/history explicitly; do not clear
   a whole bank, replay old motion or double-count old/new contributions.
4. Preserve surviving positions, velocity and unrelated numerical/motion state
   at the boundary. Schedule necessary new/affected work through the existing
   binary type phases. Preserve untouched centroid residue and activity.
5. Clear released references/relationships and native presence/activity as part
   of the coherent change. Publish counts, bindings and one committed generation
   together, then continue ticking the same prepared execution.

Universal mass and selected focal fields have global effects. Changing them can
legitimately affect particles outside a directly edited membership. Test those
dependencies separately from truly unaffected sleepers; do not promise all old
activity bits remain unchanged. Preserve the accepted particle calculation,
post-particle contact propagation and centroid propagation order, and evaluate
all connected fields of a woken particle. No extra scheduling state is introduced.

### Adjacency and required calculation cost

Keep the current stable CSR relationship representation as the first baseline.
Refactor its builder to use reserved counts/offsets/order buffers, accepting live
records and capacity strides separately. Reuse native facilities where available;
bounded native C++ structural indexing is permitted where the existing path has
no equivalent exposed Taichi primitive. Do not add a Python frontend dependency.

A batch may require rebuilding indexes across live relationships and their
bounded record headers. Report that work and its transfers explicitly; it is
not a single declaration assignment. Preserve structural reduction order across
holes/reuse rather than sorting by whichever numerical slot was available.
No per-item layout rebuild, full-study projection or scan over potential space
is acceptable. Incremental indexing can follow measured need; a new custom
traversal/scheduler is not the baseline solution.

Changed structural contributions require current group and universal masses,
not the cached publication-only values. Reuse loaded data for related derived
updates while hot, following Patrick's declaration/calculation/allocation rule.
Existing force laws, precision rules, participation and physical extents remain
the numerical contract; this segment adds no replacement mechanics or epsilon.

## 4. Limits, failures and observations

Return a bounded capacity-exhausted result naming the limiting budget, demand
and available reserve. Invalid references/input and budget exhaustion preserve
the previous committed state, readiness and generation. Padding is never usable
overflow capacity. No automatic backing growth, new study or recipe replay is
an ordinary recovery mechanism. Explicit reservation expansion is outside this
segment; defined exhaustion satisfies its finite-resource boundary.

A backend failure after mutation begins is a different case: recover only if
prepared scratch can prove restoration. Otherwise mark execution unready and
refuse current tick/report use; a prior inspection is historical, not a healthy
current result. Test this boundary without claiming general GPU fault recovery.

Provide a minimal native inspection of the committed generation and current
bindings/counts/values. Reserve its bounded output storage. Existing ActiveReport
serialization and production owner/session integration are later work; report
formatting/readback costs stay separate from native update/tick costs. Storage
reuse cannot make a stale reference refer to a different object.

Measure setup, first nonempty use, each ordinary batch, required calculation,
inspection and rejected operations separately. Record:

- Potential declarations, prepared bytes/padding and each typed capacity; present
  records, live edges/contacts/references and awake/evaluated work separately.
- Runtime requested bytes, native chunks/list growth, pool request/raw request
  counts, raw reservation, host aligned cursors, SNode/tree and compiler activity.
- Host control/staging/launch metadata allocation and visits, update/index work,
  transfer bytes and synchronization/launch costs. Audit repeated shape-vector
  and context construction; reuse supported native binding storage.
- Backend-specific gaps: general host heap, first-write faults/residency, device
  alignment and CUDA async driver allocations are not covered by #161's pool
  counters. Add appropriate observation before making claims about those costs.

The required prepared execution path obtains no new native backing, rebuilds no
layout and compiles no code. Required recurring host metadata allocations must
be exposed and addressed within this boundary, not relabelled as setup. State
remaining instrumentation/backend limits precisely; native counter stability is
not a universal allocation-free or physical-residency claim.

## Delivery sequence and acceptance

1. **Mapping and budgets:** prove general native record binding/reuse, fixed
   strides, all budget formulas and stale-reference behavior. Report metadata
   growth and any narrow native adapter before wider kernel changes.
2. **Empty prepared execution:** provision every supported family/buffer/list and
   dispatch; finish empty. Capture setup versus first-use counters on both host
   paths. Replace initial-count constants in reset/seed/index work appropriately.
3. **Numerical fixture execution:** tick existing letter/byte fixture forms, apply
   repeated definitions/relationship changes/releases through the internal
   boundary, then tick again. Use actual existing kernels, not copies of the
   equations or another symbolic four-record demonstration.
4. **Failure and handoff:** verify each budget independently, retain evidence and
   focused CPU regressions, then submit the implementing PR for review with
   explicit local GPU commands and counter coverage. GPU execution/driver tracing
   remain a separate local validation stage, not a claim from CPU success.

Proposed evidence profiles are test configurations, not installation defaults:

| Profile | Potential `V` | Numerical `P` | Fields `G` | Edges `E` | Contacts `C` | Max batch |
|---|---:|---:|---:|---:|---:|---:|
| Padding and boundary cases | 1,048,576 | 70 | 9 | 280 | 140 | 32 |
| Larger working set | 16,777,216 | 4,096 | 257 | 16,384 | 8,192 | 256 |

Before running, publish finite `R`, focus and scratch reservations derived from
each profile's supported operations and peak overlap, not guessed from current
live counts. Exercise at least 32 update/tick cycles, with noncontiguous potential
addresses and payload holes/reuse across native blocks. Actual chosen reserves
and any justified profile changes accompany source/backend identities.

Acceptance must include:

- Empty ready state with zero mass, live numerical work and relationships;
  padded native slots remain inaccessible. Logical potential space greatly
  exceeds numerical reserve without a numerical payload per empty entry.
- Exact forward/reverse mappings, distinct repeated occurrences, mixed field
  kinds and preserved contribution/ordinal order under nonmonotonic reuse.
- Ticking before and after repeated batches; stable surviving positions,
  velocities, motion history and untouched centroid residue at commit. Add
  contributions to old fields and genuinely new fields/edges/contacts.
- Independent expected universal/group masses and inclusive centroids, correct
  focal references and wake behavior. Include true unaffected sleepers and a
  separate universal-mass/focal-origin case with broader required activation.
- Contact peers on both sides of source indices and in unused blocks; no peer
  calculations during the wrong type phase. Sleeping presence survives awake-bit
  consumption; released records contribute no mass, field or contact work.
- Every capacity individually exhausted, including references, adjacency and
  scratch while particle slots remain free. Reject malformed/duplicate/stale
  references and unsupported updates without partially committing a batch.
- First use of reserved blocks, repeated release/reuse and actual nonempty
  dispatches leave the required native backing/layout/compiler counters stable.
  Inspection always describes a complete committed generation.
- BulkCopy and CpuDirect CPU checks; the existing native regression suites.
  The matched local CUDA run must verify its actual backend and record driver
  memory-pool mode/trace separately. No cross-backend trajectory identity or
  unmeasured capacity/latency guarantee is required.

## Review requested

Review this bounded engine foundation, its native mapping/reuse proof, combined
budgets, coherent update/failure rules and evidence profiles. Implementation may
refactor the named native storage/bindings and add a narrowly justified native
adapter; it must preserve the existing model and activity contract. Database
contents, shared warm storage, private-manager boundaries, the viewport and
production initialization conventions are unchanged by this segment's scope.
