# Prepared execution: backing and reusable bindings

Status: **accepted in #165, 2026-10-21; implementation under #162**.
This develops the second delivery
step of the [accepted #163 plan](prepared-capacity-foundation-plan.md), under
[#162](https://github.com/Human-Cognome-Project/human-cognome-project/issues/162).
Baseline: `main` at `41310a512a137712651720b7bea5012238079900`.
The accepted PR changed documents only. The
[CPU backing/binding implementation](../engine/docs/PREPARED-EXECUTION.md) is
accepted and merged in #166 at `4e7b193`, including the originating-context and
target-ABI correction. All eight hosted checks and all 37 CPU suites passed.
The [coherent-update section](prepared-coherent-update-plan.md) was accepted
in #167 at `284b45d`; its [bounded implementation](../engine/docs/PREPARED-UPDATES.md)
is submitted for review under #162.

[The final #165 review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/165#issuecomment-6328267055)
releases implementation and requires explicit completion ownership and cleanup
of persistent launch resources on destruction and setup/dispatch failure.

[#164's final review](https://github.com/Human-Cognome-Project/human-cognome-project/pull/164#issuecomment-6326052999)
accepts the native mapping/adapter step, with all eight hosted checks and all
30 CPU suites green. It releases combined backing/scratch, reusable bindings
and subsequent real-kernel updates. Its accepted mapping still has a present-row
lookup scan, worst-case `O(P)` serial GC per release, 16 C++ allocations / 104
requested bytes per sampled command, and temporary CUDA driver launch buffers.
These costs carry into this section; mapping acceptance does not clear them.

## Deliverable

Prepare one finite execution with actual particle, field, participation, contact,
reference, focus, native presence/awake, adjacency, staging and inspection
backing. Bind and warm its supported operation families once. Preparation ends
empty: zero present records, relationships, awake calculation entries and model
mass. The ready boundary means those storage/binding resources are prepared;
it does not mean a nonempty physical construct or focal selection exists.

Then load labelled numerical fixtures through a test-only adapter and exercise
the existing field kernels. This first nonempty execution proves that the
combined bindings work and exposes remaining lazy allocation. Coherent changes
to a ticking construct remain the following delivery step of #163, with its
survivor, mass/focus, reduction-history and failure requirements intact.

## 1. Capacity and presence in the existing engine

Keep runtime/binding work in `engine_support` and numerical/tick work in
`field_core`. Extend the existing Harness/Activity implementation; do not add a
second solver or copy the force equations into a new execution class.

| Surface | Change in this section |
|---|---|
| [PreparedCapacity](../engine/src/field/prepared_capacity.h) | Extend checked limit/layout arithmetic into an inventory of actual backing, workspace copies and launch resources. |
| [Harness](../engine/src/field/field.h) and [kernels/bindings](../engine/src/field/field.cpp) | Use fixed capacity strides and stable arrays; give live counts, capacities and occupied rows distinct meanings. Audit seed, clear, universal mass, force, contact, integration, determine and centroid paths. |
| [Activity](../engine/src/field/activity.cpp) | Prepare flags/deltas, selected focus, CSR and reset/index workspace together. Retain one native particle awake set and one centroid awake set. |
| [NativeSet](../engine/src/field/native_set.h) | Provision every usable pointer block and traversal-list high-water mark before ready; clear leaf activity without releasing prepared backing. |
| [PreparedRecordMap](../engine/src/engine/record_map.h) | Retain accepted context/incarnation validation and native recycling. Provide bounded batch use beneath the later coherent update boundary. |
| Native launchers and LaunchContextBuilder | Prepare immutable shapes, parameter paths/offsets, pointer descriptors and backend launch buffers once. Reuse them for the supported engine signatures. |

Use the accepted map's presence for particle records. Prepare independent bounded
field presence and reuse metadata using the same native facilities; a field adds
no solid body or second potential span. Relationship ledgers retain exact
occurrences and their structural order. Sleeping particles retain presence and
participation when their awake bit is consumed.

Every numerical array uses its reserved capacity as stride. An index below a
live count is not proof of presence; a hole or padded row is never an item.
Seed, mass and all-present invalidation must select present rows. Ordinary
selective work continues through the existing native type phases. Bounded CSR
construction may visit working-capacity headers; no numerical discovery scans
the potential declaration span `V`.

Preserve the fixed-count constructor and existing tests as a compatibility path
with capacity equal to supplied slots. Add explicit capacity/stride/live
accessors rather than silently changing old getter semantics. FixedStudy and
ActiveSession retain their historical fixture contract until later integration.

## 2. Concrete reservation inventory

Allocate the 22 numerical array families already enumerated by PreparedCapacity
at `P/G/E/C/F` strides. Add native layouts and every actual host/device workspace
to the same reservation manifest. Distinguish logical counts, native padding,
requested bytes, aligned cursor consumption and raw reservation.

For this bounded interface, count reference demand as one particle/field binding,
two endpoint occurrences per participation/contact, and each selected-focus
reference:

`reference demand = present P + present G + 2 live E + 2 live C + selected F`.

An endpoint is charged even if it repeats another endpoint. Source strings and
external owner identities remain outside this internal interface. Relationship
row/incarnation metadata is separately charged to its typed header storage.
This is execution-scoped bookkeeping, not a new cold address or general identity
contract. Existing arrays may supply a reference representation; do not allocate
a duplicate handle object solely to count it.

Retain the accepted evidence profiles, with explicit reference and focus limits:

| Profile | `V` | `P` | `G` | `E` | `C` | `F` | `R` | Batch `B` |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Padding | 1,048,576 | 70 | 9 | 280 | 140 | 9 | 928 | 32 |
| Larger working set | 16,777,216 | 4,096 | 257 | 16,384 | 8,192 | 257 | 53,762 | 256 |

These are test reservations, not runtime defaults. Smaller independent `R/F/B`
limits must also be testable while particle slots remain available.

Use the following workspace inventory to derive scratch bytes. Counts below are
per retained copy, before native padding/alignment; the manifest must multiply
by the actual number of host/device and committed/prospective copies.

| Workspace | Required extent |
|---|---|
| Staged command, validation/result and retired-record arrays | `B` entries of each actual fixed-width type; include batch-local reference resolution. |
| Committed/prospective structural headers and reference representations | Bounded by `P/G/E/C/R/F`; retain current and candidate metadata separately where validation/commit needs both. No copied survivor position/velocity bank. |
| One prospective CSR copy | `6P + 2G + 2E + 2C` i32 values: particle/group offsets, both contact offset directions and four order arrays. Committed CSR storage is already among the 22 families. |
| Sequential CSR builder counts and cursors | Two i32 arrays of `max(P,G)` logical entries, reused across the four builders. |
| Validation/duplicate/affected-header scratch | Bounded by `P+G+E+C` entries with exact element types declared in the manifest; never an additional persistent awake bank. |
| Candidate derived numerical results | Up to `8G+1` f32 values for group values and universal mass; this reservation does not select the later treatment of changed reduction history. |
| Native inspection output | Fixed header plus capacity-bounded particle, field, participation, contact and focus rows, using the actual inspection struct sizes. |
| Prepared parameter storage | Per-operation host argument/result buffers and descriptors; on CUDA, persistent device argument/result buffers sized from compiled signatures. |

Define `ScratchBytes` from the checked sum of workspace extents times actual
element sizes and copy multiplicities. Print that finite value and every term
before acceptance runs; do not choose an unexplained KiB/MiB allowance. Native
sentinels use `max(count,1)` per physical array without admitting logical items.
Include any extra temporary arrays required by the implementation in the sum
before testing. No hidden fallback allocation is an exhausted-scratch response.

Check known shape products, byte sums and padding before allocation. Complete
the manifest with actual native layout/chunk/allocator measurements before
declaring ready. Backend/setup failure leaves no usable prepared execution.
Native slack and page residency remain distinct from logical scratch demand.

## 3. Prepare supported launch bindings

The current wrappers create shape vectors and launch contexts repeatedly.
The native CPU launcher also builds parameter-index vectors, and CUDA builds
temporary pointer maps and allocates argument/result buffers each dispatch.
Fix those concrete surfaces for the prepared path.

During preparation, validate and retain the engine's supported flat scalar and
ndarray parameters, shapes, argument offsets, pointer descriptors and compiled
launch metadata. BulkCopy binds stable backend arrays; CpuDirect binds fully
reserved host storage with lifetime through synchronization. Cache scalar update
locations through native typed machinery, not unvalidated ABI offsets in the
wrapper. Values/counts may change; layouts and borrowed addresses do not.

Use existing native facilities first. Where the current launch API cannot retain
these resources, add a narrowly scoped native prepared-binding extension. It
must use existing Program/device ownership and allocation APIs, not another
memory pool, scheduler or general frontend. Preserve the ordinary launcher for
other signatures; reject unsupported prepared signatures at setup.

For CUDA, allocate persistent device argument/result buffers during setup and
reuse them on the existing ordered stream. Include minimum result-buffer needs
even for kernels with no explicit returns. Support only explicitly bound backend
arrays on this path; implicit host-to-device external-array staging cannot hide
temporary allocations. Borrowed host data, async copies and buffer reuse must
obey completion/lifetime rules. One synchronized engine owner is sufficient;
no overlapping submission queue or extra epoch/bank machinery is required.

CPU acceptance must show that the observed recurring 16 C++ allocations have
been removed on the supported prepared dispatches, including scalar updates.
Measure the combined wrappers and native calls, not only an isolated launcher.
Expose remaining direct malloc or backend allocations separately. CUDA source
builds are useful checks; actual driver allocation/behavior qualification remains
local under #145 and cannot be inferred from CPU success.

## 4. Batching and native recycling

Reserve `B` input/result entries now so the later coherent updater does not
compose a stream out of synchronized one-record map calls. Validate bounded
references and shapes before claiming native indexes. Carry forward duplicate,
foreign-context, incarnation, padding and exhausted-budget checks.

For a validated retirement batch, recycle through the existing native recycled
list, then collect once per affected allocator at a quiescent boundary. A narrow
recycle/collect split may be added to the accepted adapter if needed. Do not add
another free queue. Preserve the current single-record wrapper for its tests.
Report collection and lookup work explicitly: batching may amortize GC; it does
not make lookup constant-time or remove native metadata work.

Preparation must include recycled-list/free-list high-water marks and peak
pending claims. A retiring record is unavailable for reuse while any admitted
kernel or candidate validation still needs it. Validate peak demand separately
from final counts; a full reservation cannot rely on an uncommitted release to
make a claim safe. This section prepares these resources and bounded operations;
the following coherent-update step still owns their multi-family commit rules.

## 5. Preparation and first real execution

1. Check limits and byte arithmetic; construct stable host/device arrays, native
   maps, presence/awake layouts, workspaces and launch bindings.
2. Provision native backing and traversal-list high-water marks for every usable
   block. Storage-only setup may exercise native masks, but no physical kernel
   runs against absent records whose temporary setup masks are set. Finish with
   zero presence, relationships, awake bits and mass.
3. Compile/register every supported kernel family and dispatch its guarded empty
   path: map/batch validation and recycling, CSR/indexing, mass/centroid seed,
   activity, force/contact/integration/determination, inspection and rejection.
   Preserve SNode-specific cache names. Zero-work dispatch alone does not prove
   the backing for nonempty list/container paths; provision those explicitly.
4. Publish the empty ready inspection and measured resource baseline. Empty
   focus is not evidence of a production bootstrap/focal convention.
5. A test-only fixture adapter populates bounded rows while execution is
   quiescent, using supplied starts solely as labelled test values. Run actual
   existing kernels with particle/field holes, relationships in both contact
   directions, mixed field kinds and previously unused native blocks. Compare
   mass, centroid, force/contact and motion results with established fixtures.
   Measure this first nonempty use separately from setup and later ticks.

Keep one preparation and one bound execution throughout this proof. No hidden
Harness rebuild, fixture republish or survivor download/reupload may stand in
for reusable bindings. Stable repeated ticks must retain numerical history;
sleeping contributors remain in mass and centroid calculations. Preserve the
particle, post-particle contact and centroid phase order and precision rules.

## Acceptance and following work

Require BulkCopy and CpuDirect CPU evidence for both profile capacities, empty
ready state, first nonempty real-kernel use and at least 32 subsequent ticks.
Exercise independent backing/workspace/ref/focus/batch limits, boundary padding,
last usable records, stale/foreign bindings and repeated prepared-buffer use.
Rejected setup/input cannot produce a ready partially prepared execution.
Unexpected dispatch failure marks execution unready; old inspection is historical.

Retain source/backend identities, exact reservation manifests, setup/first-use/
ordinary-dispatch counters, host allocation observations, transfer and collection
costs, and the full existing native regression suite. Within the tested prepared
boundary, require no new native backing, layout rebuild or compilation, and no
recurring C++ launch/control allocation. Keep page-fault/residency and unobserved
malloc/driver limits explicit. Local CUDA commands must verify the actual backend
and record driver/pool mode and allocation traces.

Submit implementation in reviewable backing, native binding and real-kernel
verification commits. A complete coherent update proof follows: apply repeated
relationship/record changes while ticking; preserve survivors and untouched
centroid residue; handle changed mass/focus dependencies; reject invalid batches
without partial publication; retain backend-failure evidence. This section's
single initial fixture load and stable ticks do not satisfy that later proof.

#162 and #156 remain open. Production placement/bootstrap conventions, source and
report integration, general LoD exchanges, #115's exploratory coupling note,
databases and private-manager behavior remain outside this section. All runtime
and test implementation stays native C++; there is no operational Python work.
