# Private exchange-owner resource inventory

Accepted through #235 on 2026-11-08; [hosted acceptance](../tests/admission/README.md#maintainer-integrated-acceptance--228235-2026-11-08)
records the integrated stack. Remaining limits below are still open.

This inventories the private `assembly::detail::ExchangeOwner` on the explicit
integrated #228/#230–#235 stack. It accompanies the retained-capacity census and scoped
adoption lifetime witnesses; it does not claim complete machine peak measurement,
a complete byte-budget admission gate or complete preparation-work
enforcement, allocator accounting or GPU qualification. Those remain required
before complete owner-resource acceptance. The harness remains borrowed and the
single readiness/generation/publication authority is unchanged.

## Retained capacity implemented in this slice

`ExchangeOwner::storage()` returns separate `ProjectionStorage` charges for the
following families. `OwnerStorage::total()` checks their sum before returning it.
Reporting reads metadata only, does not allocate, and requires the same exclusive
access/lifetime boundary as other owner operations. It is private inspection,
not an operational memory-budget policy or a second state authority.

| Family | Owned surfaces counted |
|---|---|
| `owner` | `sizeof(ExchangeOwner)`, including embedded references, smart pointers, vector/map headers, generation/gates and fixed reports |
| `projections` | Both complete projection objects and their existing recursive census: recipes, inputs, occurrences, exposures, active and spare parents, nested keys/pins/text |
| `maps` | Outer capacity of all six stores: occurrence/field maps in owner, binding and coordinator; nested UUID/path/address storage of **every constructed slot**, including inactive slots |
| `binding` | Binding object; mass, outgoing handle, captured source-edge buffers; outgoing-record and edge masks |
| `coordinator` | Coordinator object; command, intrinsic, field-command-index, hierarchy-node, child and contribution buffers |
| `declarations` | Shared-pointer table capacity; both declaration objects per selection; owner/recipe pins; incoming/outgoing descriptors, their parents, ordered leaves and nested keys/pins |
| `memberships` | One catalogue object, study/recipe pins, all-alternative contribution capacity and constructed rows' occurrence/field/target keys and pins |

Object sizes include embedded member objects. Vector capacity is multiplied by
element size once; nested backing is then counted only for constructed elements.
The six map vectors deliberately construct reserved slots, so their nested
backing is counted even beyond the active length. Other unused vector slots have
no constructed nested storage. `vector<bool>` capacity is in bits and is rounded
up to bytes. Text includes capacity plus a terminator, conservatively including
inline string capacity already represented inside object sizes. Checked addition
and multiplication reject overflow without publishing a partial report.

Binding aliases do not duplicate declaration/catalogue pointee charges. The
owner creates distinct declarations and a single catalogue; `shared_allocations`
records their control-block count (two per selection plus one). Control-block
**bytes** and allocator metadata are implementation-dependent and are not included
in the retained-capacity charge. The smart-pointer objects themselves are already
counted in the containing objects/table. This report must never be labeled heap
bytes, RSS, total process memory, provisioned memory or a setup peak.

## Ownership, borrowing and cleanup

Owner construction allocates the committed projection, spare projection,
coordinator and its binding. These unique owners release their subobjects on
teardown. Publication swaps whole projection pointers and map stores; ownership
of the same pools persists while their committed/staging roles rotate. Clearing
logical lengths, rejection and consumed plans do not release their reservations.

The declaration table and membership catalogue have shared ownership. Binding
references extend their lifetime and release the preceding alias on preparation.
In ordinary owner teardown, the owner's membership pointer and declaration table
are destroyed before `plan_` (reverse member declaration order); binding-held
references keep the selected declaration and catalogue alive until binding
destruction. Test-held shared references can extend those lifetimes further.
They must be released before measuring complete reclamation. There is no ownership
cycle: declarations and catalogue do not point back to owner/binding/coordinator.

The harness reference is never deleted by the owner. Native particle/field claims,
prepared source arrays, hierarchy/history/contact backing, Taichi arrays, runtime,
compiled kernels and transfer buffers remain harness/runtime resources. Destroying
the owner neither retires live native objects nor makes a new healthy execution.
`ExchangeIncoming`, new-field inputs and caller hierarchy arrays are borrowed
during preparation/sealing; the owner copies the needed values into its reserved
buffers. Caller fixture/recipe/input storage is not owner-retained storage.

Constructor failure unwinds already-constructed member subobjects and local
temporaries. No completed owner is returned and adoption performs no native
publication. During preparation, failure invalidates pending intent but may retain
partial candidate metadata. Committed owner identity and mappings remain intact;
retry overwrites candidate storage. A late failure after native mutation makes
the harness unready, not rolled back. Inspecting retained host capacity in that
state does not certify valid native state or recovery.

## Adoption overlap and setup-peak inventory

The retained report is a final-capacity snapshot. It cannot reconstruct the maximum
simultaneous allocations during the following phases in `exchange_owner.cpp`:

1. By-value projection/row/field arguments may coexist with caller originals.
   Moving the projection into its unique object does not remove caller copies;
   constructing owner maps copies argument rows/fields and their nested keys.
2. `MembershipFields::collect` builds a tool lookup map, traversal state map,
   reached-target maps/sets, aggregate-owner map and recursive traversal/function
   state while growing the output catalogue. Map/set nodes and copied keys are
   setup costs; graph depth also consumes call-stack space. These disappear on
   return/unwind, while catalogue capacity survives.
3. Each selection builds forward descriptors and ordered leaves from the pinned
   recipe. `declare_letter_exchange` uses aggregate/parts/source temporaries and
   growing vectors. The reverse declaration initially copies the forward one.
   Both coexist with retained declarations from previous selections.
4. The temporary adoption `ExchangeBindings check` allocates all binding buffers
   and reserved maps for validation. Its by-value row/field arguments, trusted
   field-map copy and implicit staging-map conversions also coexist during the
   constructor call. It shares the current declaration/catalogue; it does not
   duplicate their pointees. The check is destroyed each loop iteration.
5. The second complete projection is copied, then both exposure views reserve
   active/spare parent and exposure backing. Growing vectors can briefly hold old
   and new allocations simultaneously; final `capacity()` omits the old blocks.
6. The persistent coordinator/binding allocate their buffers. Reservation of all
   three occurrence/field map pairs grows outer and nested backing. Declaration
   table growth and all `make_shared` control blocks are additional setup costs.
7. Constructor parameters and last temporaries release before adoption returns.
   The final report covers surviving owner payload capacity, not this peak.

A future peak witness must include allocation/deallocation pairing through all
these phases, constructor failure at allocation points, and teardown back to its
baseline. Separate owner-attributable allocations from pre-existing caller and
borrowed harness resources, while also reporting simultaneous combined occupancy
where a machine budget requires it. Record requested allocation bytes separately
from allocator/residency measurements; neither follows from capacity arithmetic.
The census alone supplies no fail-allocation or reclamation witness. The accepted #232
separate test-only contribution measures a bounded adoption scope.

### Adoption allocation/failure witnesses (accepted #232)

`owner_lifetime.inc` brackets the entire `ExchangeOwnerAccess::adopt` call,
including by-value input copies, owner allocation, constructor temporaries,
shared allocations and nested backing. Caller originals and the already-installed
harness predate the scope. The existing one- and four-occurrence owner fixtures
each measure successful adoption's requested C++ allocation bytes still live at
return and maximum simultaneously live requested bytes during adoption. These
are distinct from the conservative retained-capacity charge, reported alongside
them without asserting equality.

The test then destroys the owner while retaining one declaration and catalogue
alias. It checks that tracked storage remains until those aliases are released,
and that live tracked blocks/bytes return to zero after the last release. It
injects `std::bad_alloc` at every allocation ordinal in the successful adoption
trace, requiring the failure to be reached and all preceding allocations reclaimed.
Every failed attempt preserves readiness, generation and tick count; a final
successful adoption/reclamation and exact native snapshot comparison verify retry
and unchanged borrowed execution. The existing 32-exchange/tick sequence follows.

The observer is test-only and opt-in for `live_hierarchy_test`. A fixed 65,536-slot
thread-local table tracks pointer/request-size pairs without allocating itself;
overflow fails closed. Deleted entries preserve probe chains. Scalar, array,
aligned and sized-delete forms pass through the existing allocation interception.
Controls independently check exact requested-byte totals, untracked pre-existing
deallocation and an injected failure. Reporting occurs after observation ends.

This is a current-thread C++ `new`/`delete` witness: adoption is synchronous and
performs no native launch. It includes allocations used by this standard library's
shared control blocks, without separating their layout-dependent overhead from
pointees. It excludes arbitrary `malloc`, runtime/driver allocation, other threads,
stack/exception-runtime storage, allocator metadata, page residency and RSS. It
does not measure concurrent callers, native setup, GPU cleanup or process peak.
Zero tracked allocations establishes reclamation within this scope, not a general
leak-proof guarantee. Exhausting one deterministic successful trace is not an
exhaustive test of all recipe graphs or all error paths. Byte/preparation-work
budget enforcement and broader resource qualification remain required.

## Work inventory and remaining budget gate

<a id="hierarchy-sealing-gate-contribution-awaiting-review"></a>

### Hierarchy sealing gate (accepted #233)

Owner and coordinator sealing now require an explicit `HierarchySealBudget`.
There is no production default or unlimited overload. Existing test facades
request the exact calculated charge to retain their original fixture behavior.
`hierarchy_charge` inspects counts and reserved capacities only, in constant
metadata work; it never reads a caller table or allocates. Checked products and
sums return `Overflow` with an empty charge if any result cannot be represented.

The per-call byte allowance covers the simultaneous **caller input spans plus
retained destination capacities** for nodes, child indices and contributions.
Caller spans use count times element size; destination buffers use their actual
reserved capacity times element size, including unused slots. They are reported
separately and conservatively added even if a trusted caller aliases storage.
This gate does not authorize or undo the destination allocation at adoption.
It authorizes copying into that existing storage while those input spans coexist.

Copy work is one unit per input payload byte. This is a structural copy-work
charge, not measured CPU instructions, elapsed time, a hierarchy-validation work
budget or native execution's existing `hierarchy_work` allowance. The small,
constant admission calculation is outside the payload-copy charge. Source
tables are copied only after both allowances pass and the existing pointer/count
and native-capacity checks pass. `ByteExhausted`, `WorkExhausted` and `Overflow`
return without allocating, copying, sealing, consuming the pending operation or
changing owner/native state. A valid request can retry with exact allowances.
Existing malformed-input exceptions and exclusive-access requirements remain.

The one-occurrence sequence checks these outcomes in every exchange direction,
including deliberately unreadable source pointers on under-budget calls,
one-unit-short allowances, overflow without a partial report, unchanged backing
and native snapshots, and exact-budget retry. Both existing owner sequences and
standalone coordinator fixtures use the same production gate.

This is the first bounded preparation-budget operation, not a combined owner
budget. It explicitly excludes caller allocation slack/other inputs, fixed object
headers, all other owner components, adoption transients/control-block overhead,
borrowed native storage and runtime/driver resources. Native limits continue to
govern native work; these surfaces must be reconciled by a future combined policy
without counting the destination buffers twice. Adoption-byte limits and the
remaining preparation scans/key copies still require gates before their work.

<a id="source-capture-work-gate-contribution-awaiting-review"></a>

### Source-capture work gate (accepted #234)

`ExchangeOwner::prepare`, binding preparation and source capture require an
explicit capture-work allowance. This allowance covers capture only, after
declaration selection, projection staging and identity-map staging. Those earlier
steps still need their own gates. The following command gate supplies the later
coordinator allowance separately.
Adoption/standalone binding construction explicitly uses the maximum capture
allowance because setup-work admission is not yet implemented; this is not a
claim that adoption is bounded. Test facades retain convenience allowances.

`CaptureWork` (an alias of the shared `PreparationWork`) charges incrementally
before the associated operation:

| Surface | Structural work units |
|---|---|
| Fixed capture admission | One; zero allowance stops before clearing capture buffers or scanning source payload |
| Occurrence/field/source/parent rows, scalar edge matches and duplicate-field candidates | One per visited row/candidate; fixed scalar checks and assignments are included in their enclosing row |
| Identity comparisons in projection/tool searches, duplicate occurrences, source/field resolution and catalogue filtering | One dispatch unit plus both operands' UUID text lengths and nested path/address byte lengths, charged before equality reads payload |
| Captured mass/edge append | One each, before append |
| Mask initialization | One per logical mask element, reserved before either assignment |
| Final source validation | The existing full binding validation work charge, reserved before that scan |

Charges are conservative structural units, not CPU instructions or elapsed time.
Fixed container/key-length metadata inspection and meter bookkeeping are not a
separate measured workload. A multi-part comparison may prepay some units before
another component exhausts the allowance; `charged` means reserved units, not
completed useful work. Checked addition/multiplication returns `Overflow` without
wrapping. `WorkExhausted` reports charged units and `next_required`, the minimum
cumulative allowance for the next denied charge, not a prediction of the full
operation's eventual total.

An internal trivial unwind token stops nested searches and is caught at capture.
The result is returned to the owner, whose pending gate was already invalidated.
Capture remains invalid, even if some reserved buffers now contain partial data;
committed owner maps/projection/reports and native state remain unchanged. A new
prepare overwrites those buffers and retries. This preserves existing validation
exceptions for invalid input. The tests observe no C++ `new` during exhaustion,
but exception-runtime allocation/stack handling is outside that observation and
must be accounted for by a future combined memory policy.

The single-occurrence sequence checks zero allowance, one-unit-short late
exhaustion and exact successful-charge retry in both directions. Its first
exchange traverses every successive charge boundary using `next_required`,
checking invalid partial binding/pending state, progress and unchanged committed
native state. Arithmetic witnesses cover product/sum overflow. All source-matching
predicates and native numerical oracles retain their previous meaning.

<a id="command-construction-work-gate-contribution-awaiting-review"></a>

### Command-construction work gate (accepted #235)

Owner preparation now requires separate capture and command-work allowances.
`OwnerPreparationResult` identifies `Capture`, `Commands` or `Complete` and
retains each phase's charge/result separately. `Complete` means these two preparation phases completed, not
native publication or a sealed hierarchy; explicit hierarchy sealing remains
required. When capture stops, command work
has not started; its zero result must be interpreted with the phase, not as a
successful command plan. When command work stops, capture may still describe
valid committed-source metadata, but the coordinator is consumed/unsealed and
the owner has no pending operation. A successful retry rebuilds the candidate;
no partial commands, mappings or hierarchy can be published.

The existing checked structural meter is shared in `preparation_work.h` in a
separate mechanical commit. Capture aliases preserve its prior behavior. Command
construction charges before the following work:

| Surface | Structural work units |
|---|---|
| Admission and bound-source revalidation | One fixed unit, then the existing complete binding-validation charge before its scan |
| Field/occurrence map copies and incoming keys | Binding object size plus one key-copy dispatch and nested UUID/path/address payload bytes before each copy |
| Field-command indices | `sizeof(int)` per assigned/appended element |
| Descriptor matches and field/key searches | Row dispatch plus conservative operand payload charges through the shared equality meter |
| Command and intrinsic append | `sizeof(UpdateCommand)` / `sizeof(PlacementIntrinsic)` before each append; fixed value construction is part of the enclosing row's structural work |
| Outgoing bodies/edges, contacts, parents, membership fields/rows, field-use and command scans | One per scalar row visit, plus variable-length comparison work when needed |
| Field retirement | Retained-slot rotation and parallel index shift charged for the suffix length times binding-object-plus-index size before either erase; nested buffers move with slots rather than copying payload |

The meter catches exhaustion/overflow at command construction, retaining only
unusable partial candidate metadata. Zero allowance invalidates gates before
source validation or logical buffer clearing. Existing capacity/invalid-input
errors retain their meaning; native execution/publication and numerical predicates
are unchanged. Internal stop-token exception-runtime costs remain outside C++
`new` observation, as for capture. Standalone coordinator construction explicitly
uses the maximum command allowance pending its separate setup-work gate. Test
facades provide convenience allowances; the production owner API has no defaults.

Tests check distinct phase results, zero/late-short allowance, every successive
first-exchange charge boundary, unchanged committed/native state and backing,
consumed/unsealed candidate state, and exact two-allowance retry. An independent
key-copy witness checks nested path payload charging. Existing capture overflow,
lifetime, repeated-exchange and numerical oracles remain active.

This gates command construction, not declaration selection, projection exposure
staging, initial binding-map staging or adoption. Work-unit charges can depend
on ABI object sizes and comparison paths and are not CPU instruction/time claims.
They are not memory reservations or additions to the retained-capacity census.

### Combining resource domains remains outstanding

The retained owner census is counted once in any future combined memory policy.
Hierarchy sealing's destination capacity is already in that census, so its
per-call combined charge must not simply be added again. Caller input spans and
actual simultaneous setup temporaries are additional domains; caller capacity
slack, shared control-block/allocator overhead and borrowed harness/native/driver
resources need explicit, independently sourced dispositions. Current gate totals
cannot be labeled a complete machine budget by summing them. Capture work is not
a byte charge and does not add another copy of retained storage. Command work
likewise records construction work, not extra retained memory. These gates do not
change the authoritative readiness/generation/publication boundary.

Current execution charges are deliberately narrower than complete preparation:

| Phase | Existing behavior / outstanding charge |
|---|---|
| Declaration selection | Linear scan over retained alternatives, exact key comparison; no preparation-work allowance |
| Projection staging | Resynchronizes exposure/active flags and partitions retained parent rows; no full preparation-work allowance |
| Binding capture | Incremental structural-work gate before scans, key comparisons, masks and final validation; stage-map copying and surrounding preparation remain separate |
| Binding validation | Existing `16 * (occurrences + fields + captured_edges + live_edges + 1)` charge covers its bounded read-only execution validation; it is not a charge for all prior capture work |
| Coordinator preparation | Separate incremental command-work gate covers revalidation, descriptor/key comparisons, command/map copies, contacts, membership, field usage and retirements; setup and preceding staging remain outside it |
| Hierarchy sealing | Explicit input-span plus destination-capacity byte allowance and payload-byte copy-work gate before reading/copying; caller construction, constant admission bookkeeping and native hierarchy validation remain separate |
| Execution/publication | Coordinator reserves binding work plus `16 * (next_occurrences + next_fields + 1)`; owner reserves 64 further PlacementWork units; native placement/cell/hierarchy gates remain unchanged |
| Census | Visits both recursive projections, all constructed map slots, declarations and catalogue rows; its metadata walk is inspection work, not included in the execution charge |

Complete accounting must include setup and successful/failed preparation, nested
key comparisons and copies, not just the existing execution formula. Bounds must
be checked before the work they authorize and preserve ordinary rejection/retry.
Retained payload, setup overlap, allocator overhead, borrowed native peak and
caller inputs must have explicit dispositions before enforcing a total byte
budget. Do not silently repurpose PlacementWork as a memory limit or treat the
capacity census as an allocation reservation. Budget policy and peak/work
instrumentation remain the next resource slice, ahead of more operation families.

## Factoring implications and evidence

The census lives in its own compilation unit without widening production access.
The inventory separates immutable declaration/catalogue data, reusable host
candidate buffers and borrowed execution surfaces. Subsequent #229 factoring
should follow these lifetimes, preserving the coordinator's exclusive publication
boundary and the harness's sole readiness/generation authority. It does not
justify moving numerical state into host objects or adding a second commit gate.

Focused owner tests exercise allocation-free reporting, inactive nested map and
field-command capacity deltas, aggregate overflow, shared allocation counts, and
stable totals across repeated one/four-occurrence exchanges and ticks. Numerical
oracles and operation behavior remain unchanged. See the source-pinned run record
in [admission evidence](../tests/admission/README.md).
