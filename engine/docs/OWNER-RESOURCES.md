# Private exchange-owner resource inventory

Accepted through #246 on 2026-11-10; [hosted acceptance](../tests/admission/README.md#maintainer-integrated-acceptance--245246-2026-11-10)
records the integrated stack. Remaining limits below are still open.

This inventories the private `assembly::detail::ExchangeOwner` on the explicit
integrated #228/#230–#235/#237–#243/#245/#246 stack. It accompanies the retained-capacity census and scoped
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
| `owner` | `sizeof(ExchangeOwner)` plus its separately allocated reservation-domain object, including embedded references, smart pointers, vector/map headers, generation/gates and fixed reports |
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
   table growth and shared object/control blocks are additional setup costs.
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

<a id="adoption-input-snapshot-admission-contribution-awaiting-review"></a>

### Adoption input snapshot admission (accepted #238)

A private `ExchangeOwner::adopt` factory now accepts projection/row/field inputs
by const reference plus explicit `AdoptionInputBudget` byte, inspection-work and
copy-work allowances. It runs admission before `new ExchangeOwner` and before
any by-value input argument copies. Budget rejection returns no owner and an
`AdoptionInputResult`; it neither allocates C++ objects nor changes caller/native
state. Invalid constructor inputs and allocation failures still throw and unwind.
`Complete` means **input admission completed**, not construction or native readiness.

This is deliberately the first input boundary of adoption, not a complete
constructor-byte or setup-work gate. The byte charge combines:

- Caller inputs: the recursive projection capacity census plus both caller binding
  vector headers/capacities and constructed rows' nested UUID/path/address capacity.
- Input snapshots: the same object/header structure and **logical sizes** of copied
  vectors, nested keys and strings. Text includes its terminator and conservatively
  counts inline text again, as in the existing projection census.

Caller capacity slack contributes only to the first domain. The factory's three
by-value constructor parameters are the snapshot domain; later moves do not justify
charging those snapshots again as independent retained capacity in a combined policy.
This is a conservative **payload charge**, not requested allocator bytes, a bound
on implementation-dependent copied-container capacity, RSS or machine peak.
The owner object itself, heap projection object, owner map copies, catalogue,
declarations, temporary validation, spare projection and all reservations made
inside the constructor are outside this gate and still need pre-allocation limits.
Borrowed harness/runtime, caller inputs outside these three arguments, allocator
metadata/control blocks, stack and exception-runtime overhead also remain separate.
Do not add this combined input charge to the final retained census as if they were
disjoint simultaneous allocations.

Inspection makes two allocation-free walks, capacity then logical-size, over the
same projection traversal. Each vector charges one header plus one unit per logical
row before visiting nested row metadata; scalar vectors are conservatively charged
by count even when only their size is needed. Each text header charges one before
reading size/capacity. Fixed admission/root/map headers charge one each. Character
contents and scalar payloads are not read. Existing unmetered retained census
behavior is preserved. Checked products/sums fail without wrapping; an interrupted
inspection returns only its work result, with no partial byte report.

After inspection, bytes gate first, then snapshot-copy work (one unit per snapshot
payload byte) is prepaid as a complete block before any allocation/copy. Results
identify Inspection, Copy or Complete and retain separate inspection/copy charges.
Byte exhaustion reports the full required charge and no copy work; copy exhaustion
likewise leaves all inputs untouched. Fixed arithmetic/meter bookkeeping is not
instruction/time measurement. A test convenience overload explicitly supplies
maximum allowances; the private production factory has no defaults.

Single/four-occurrence tests exhaust every inspection-charge boundary at the actual
factory under the no-C++-new guard; they also check zero/short bytes and copy work,
exact successful adoption and unchanged native state. Independent deltas distinguish
outer/nested caller capacity from populated path and recipe-text snapshot sizes.
Overflow controls preserve accumulators; the existing constructor allocation-failure,
peak, alias-lifetime/reclamation and repeated exchange/tick tests remain active.
Exception-runtime costs remain outside the allocation witness.
<a id="bindingcoordinator-allocation-domain-contribution-awaiting-review"></a>

### Binding/coordinator allocation domain (accepted #239)

The private adoption factory now also requires `ReservationBudget {bytes,
allocation_work}` and returns a separate `ReservationResult`. Input admission still
runs first; when it fails, the reservation result is unused and its zero/default
value must not be read as a successful constructor. After input admission, a small
accounting object is created. It survives constructor unwind and is transferred
to the successful owner; member order releases it only after all attached buffers.

A stateful standard-library allocator gates the following **actual allocation
requests**, including any implementation-selected capacity and bit-vector words:

- Binding masses, outgoing handles, captured edges, outgoing-row and edge masks.
  Every per-selection temporary validation binding and the persistent binding use
  the same domain; released temporaries reduce live bytes.
- Coordinator commands, intrinsics, field-command indices, hierarchy nodes,
  child indices and contributions.

Before delegating each allocation, checked count-times-element-size and cumulative
arithmetic reject overflow. The byte allowance checks currently live requested
bytes plus the new request. Thus vector growth must admit old and new blocks
simultaneously; freeing the old block later cannot hide the allocation peak.
Allocation work prepays one unit per requested byte, cumulatively across the whole
adoption/owner lifetime. Deallocation refunds live bytes, never charged work.
This is a structural allocation-request quota, not allocator CPU time or complete
constructor/setup work. No native numerical algorithm or execution gate changes.

Successful allocation updates live/peak bytes only after the underlying allocator
returns. `std::bad_alloc` propagates normally: no phantom live bytes are acquired,
while prepaid request work is not refunded. Byte/work exhaustion throws a trivial
internal stop token; the factory catches it after all constructed members and
temporaries unwind, returns no owner, and reports zero remaining domain bytes.
`required_bytes` and `required_work` describe the denied request at that point;
peak and charged work preserve the preceding history. Overflow leaves required
values zero. Fresh adoption creates a new domain and supports exact-budget retry.
Existing input-Complete still means input admission only; construction also needs
a returned owner and a successful reservation result.

The domain remains attached after adoption so any unexpected later growth still
uses its quotas. Normal preparation/execution reuses the accepted reservations.
Private standalone binding/coordinator fixtures retain an explicit default null
allocation domain and are not claimed as budgeted adoption. The owner factory has
no default reservation allowance; test conveniences explicitly supply maxima.

At the #239 boundary this is the first actual requested-byte bound for these selected buffers. It does
**not** cover the domain/owner/coordinator/binding object allocations themselves,
input copies, maps and nested keys, projection copies/reservations, membership
catalogue or declarations, their shared control blocks, graph temporaries,
allocator metadata/alignment overhead, exception runtime, stack, other threads or
borrowed harness/runtime/driver allocations. Some of those allocations/work precede
the first guarded buffer request. This is not a total-constructor or machine budget.

The retained census includes the new domain object and allocator pointer objects.
Its binding/coordinator vector capacities already represent these buffers: the
actual requested-byte report must **not** be added to that census a second time.
The report's peak also includes temporary binding buffers which are absent from
the final retained census. These distinctions remain necessary for combined policy.

Tests compare actual bit-word/integer requests and releases with the independent
C++ allocation observer; check old/new vector overlap, cumulative work after free,
product/sum overflow, and underlying allocation failure. One/four-selection owner
fixtures traverse every successive byte-peak and cumulative-work rejection boundary,
require complete allocation reclamation and unchanged native state, then retry
with exact successful allowances. Existing full allocation-failure/alias-lifetime,
input/staging/capture/command gates and repeated exchanges remain active.
<a id="map-outer-store-allocation-domain-contribution-awaiting-review"></a>

### Map outer-store allocation domain (accepted #240)

All six retained `ExchangeMap` outer vectors now use the same reservation domain
as the owner's binding/coordinator buffers. This includes the owner's initial
row/field copies and their later growth, both capture maps and both candidate maps.
Temporary validation also explicitly attaches the trusted-field map and both
staging input maps, preserving their separate copies rather than leaving implicit
vector-to-map conversions outside the domain. Every temporary binding's two reserved
outer stores likewise use that domain.

An outer request charges actual slot capacity times `sizeof(OccurrenceBinding)` or
`sizeof(ExchangeFieldBinding)` through the allocator before allocating. Old and new
outer blocks coexist in the byte admission during growth. Rejection before allocation
cannot construct new nested rows; normal constructor unwind releases earlier outer
and nested allocations. The nested string/vector **objects** are included inside
slot size; their separately allocated UUID/path/address payload is **not** tracked
by this domain. Default-slot UUID creation and nested-key reserve/copy work still
need their own limits. This is not a complete-map or complete-constructor bound.

A plain map copy is an independent caller snapshot with a null allocation domain.
It preserves every constructed inactive slot and nested reservation, but must not
retain a raw pointer to a source owner's accounting object. An explicit map-copy
constructor accepts a domain for setup-owned copies. Every production owner/setup
path in this slice passes its domain explicitly; standalone/caller snapshots are
outside owner reservation admission. Assignment reuses existing slots and retains
the destination allocator. Swapping whole stores propagates the allocator together
with its backing and logical length. Attached domains must outlive their stores;
owner member order and constructor unwind preserve this requirement.

The owner retained census already counts map outer capacity once. Comparing its
outer capacities plus the binding/coordinator buffer capacities with domain live
bytes verifies the selected retained surfaces; neither the whole nested-map census
nor the live-byte report is added again to a combined charge. Cumulative work also
includes released setup copies and old blocks, separately from simultaneous peak.

Controls check zero-byte outer rejection before any C++ allocation, growth overlap
with unchanged logical rows/backing, exact outer accounting versus larger observed
outer-plus-nested allocation, reclamation, explicit bounded copies, detached copies
surviving their source domain, destination-preserving assignment and allocator-aware
swaps. The one/four-owner byte/work boundary sweeps now cover these requests together
with the accepted buffer domain, followed by exact retry, full failure/reclamation
witnesses and repeated exchanges. Underlying allocator/exception overhead, nested
payload, projection, catalogue/declaration, object/control-block and graph-temporary
limits plus borrowed-resource accounting remained open at the #240 boundary.

<a id="shared-object-allocation-domain-contribution-awaiting-review"></a>

### Shared object allocation domain (accepted #241)

The declaration pointer table and `allocate_shared` requests for the two immutable
declarations per selection and the membership catalogue now use the owner's
reservation domain. The table charges its actual backing capacity, including
old/new overlap while growing. Standard-library allocator rebinding supplies the
actual shared allocation size, including the object, control block and any padding
inside that request. No guessed control-block size or second `sizeof(pointee)`
charge is added. Underlying heap metadata/alignment overhead remains excluded.

`SharedReservationAllocator` owns a shared reference to the domain. The owner and
factory also hold it; there is no domain-to-pointee ownership edge or cycle. This
keeps accounting valid when declaration/catalogue aliases outlive the owner, and
when weak references retain their control-block allocation after object destruction.
The charge is released on allocator deallocation, not merely on last strong
reference release. The allocator copy used for that deallocation keeps the domain
alive through the update. Raw allocators for owner-local maps/buffers retain their
existing member-order/unwind lifetime rule.
Reference release remains within the existing exclusive synchronous access
contract; shared ownership does not make the domain's counters thread-safe.

The declaration table's ordinary copy constructor selects a detached allocator.
Its independent backing is outside the source owner's quota, while copied shared
pointers still keep the original shared allocation and domain alive. Assignment
retains the destination allocator; move assignment and swap propagate it with the
backing. Owner preparation continues to reuse its retained table and shared data.

The domain itself is now created with `make_shared`; its object and own control
block remain fixed overhead outside its quota. The retained census includes its
object and embedded smart pointers, but still excludes control-block byte sizes.
`shared_allocations` continues to count only declaration/catalogue allocations,
not this accounting-support allocation. The actual domain report overlaps with
already-counted table capacity and pointee objects: do not add it to the census.

This bounds allocation requests, not the preceding `MembershipFields::collect`,
declaration construction/copies or their nested vectors/UUID/path/address payload.
Those arguments can allocate before `allocate_shared` is called. Projection and
other unique objects, graph temporaries, full setup work, domain overhead and
combined caller/borrowed accounting remain open. This is not complete catalogue,
declaration, constructor or machine-resource qualification.

Tests compare an aligned shared object/control-block request with the independent
allocation observer, retain its charge through strong and weak aliases, and check
domain release without a cycle. Zero allowance rejects before construction;
constructor failure releases bytes without refunding request work; exact fresh
allowances retry. Detached table copies and rejected growth preserve their data.
Owner tests retain actual declaration/catalogue aliases and a weak catalogue after
owner destruction, verify staged charge release, sweep constructor byte/work
boundaries, and retain full allocation-failure and repeated-operation witnesses.
On the qualified standard library, the independent observer also locates the
actual allocation containing each shared pointee and reconciles the exact sum
with table/map/buffer capacities against retained domain bytes. That witness
checks the observed colocated object/control-block layout; the production
allocator instead admits every request using the library's actual rebound type.

<a id="unique-object-allocation-domain-contribution-awaiting-review"></a>

### Unique object allocation domain (accepted #242)

Actual single-object requests for `ExchangeOwner`, both `LetterProjection`
objects, `ExchangeCoordinator` and its persistent `ExchangeBindings` now enter
the same reservation domain. Each request is admitted and acquired before its
placement constructor runs. This includes the outer owner before its by-value
constructor arguments are copied; the earlier input-admission gate still runs
first. The accounting domain's own shared allocation precedes this gate and
remains separately scoped overhead.

`reserve_unique` allocates one aligned object through the existing allocator,
then invokes a reference-capturing callback, whose closure needs no allocation,
to construct it in that storage without widening private constructor access. If allocation fails, no
constructor runs and no live bytes are acquired. If construction or an argument
copy throws, completed members unwind before the helper releases the outer block;
the whole-object destructor is not called for an incomplete object. Prepaid
allocation work is never refunded. The factory still catches budget rejection
after complete unwind and supports a fresh exact-budget retry.

`ReservedUnique` carries its allocator in the deleter. The deleter owns a shared
domain reference through object/member destruction and final deallocation, then
clears that reference so an empty reset handle does not retain the domain. This
also protects the outer owner after its own `reservations_` member is destroyed.
Move and swap carry pointer and deleter together; projection publication continues
to swap the same two objects without allocation. There is no additional pointee
control block or domain-to-owner ownership cycle. Standalone test bindings use the
same matched allocation/deallocation path with a null, unbudgeted domain.

At the #242 boundary only object storage is newly charged. Vector/string member headers and deleter
state embedded inside these objects are included in `sizeof(T)` once; separately
allocated nested payload is not silently included. Projection copy contents,
UUID/path/address backing, graph temporaries and preceding catalogue/declaration
construction still require limits. This is not a complete projection, constructor
or setup-work bound. Borrowed harness/native storage and allocator/exception costs
remain separate. All allocation and reference release is exclusive and synchronous.

The retained census already includes these five object sizes and the new embedded
deleter state. The outer returned handle and its deleter live in caller storage,
outside that owner census; the domain's own object/control-block overhead remains
outside its quota. Do not add selected live/peak bytes to retained census totals.
Shared declaration/catalogue requests and table backing retain the accepted #241
accounting and are not charged again by this extension.

Controls verify rejection before callback entry, object bytes live during member
construction, nested budget rejection, throwing constructors, underlying allocation
failure, prepaid work, exact retry, alignment, move/swap and final-domain release
on reset. The independent allocation observer reconciles each of the five object
requests with its size and the complete selected-domain sum. Owner byte/work
boundary sweeps, full allocation-failure/reclamation traces, shared/weak aliases
and repeated native operations remain active.

<a id="projection-table-allocation-domain-contribution-awaiting-review"></a>

### Projection table allocation domain (accepted #243)

The four projection tables (exposures, occurrences, active parents and spare
parents) now use `SharedReservationAllocator`. Both retained projections select
the owner's domain explicitly, covering eight table stores, initial table copies
and later parent-pool reservations. Each actual backing request is admitted before
allocation, including old/new overlap during growth and cumulative request work.
Nested row objects reside inside the slots; their separately allocated UUID/path/
address payload remains outside this gate. No second slot-header charge is added.

The owner constructor now borrows its projection argument and directly constructs
the retained copy with the selected domain. This replaces the former unbudgeted
by-value projection snapshot followed by a move; it does not add another whole
projection copy. The earlier input gate is unchanged and still runs before the
owner allocation. The spare projection uses the same explicit-domain copy path.
Study/recipe, exposure state, active flags, both parent pools, legacy input and
structural input are preserved. Recipe/input/nested copies remain unbounded by
this selected allocator, and recipe copying precedes the first table request.

Ordinary projection value copies detach all four table allocators through their
copy-selection rule. Snapshots remain usable after the owner's domain dies; copied
inactive parent rows are preserved. As before, a value copy preserves logical
rows, not unused capacity; `reserve_exposure` restores both parent capacities
before owner use. Explicit-domain copies attach each table even when it is empty.
Moves carry the allocator with the backing and retain the shared domain lifetime.
Copy assignment retains the destination allocators; assignment is not an owner
publication/transaction path. Publication still swaps two retained object pointers
and reuses their already-reserved backing.

The read-only occurrence/parent accessors now return the allocator-aware vector
type by const reference. In-tree consumers continue to use iteration/indexing;
the changed container type is not a database, identity or numerical contract.
The projection census accepts either vector allocator and continues to count
table capacity once, then nested payload separately. Its total must not be added
to the overlapping domain report. Allocator references are embedded in the
already-admitted projection object size, not separate allocations.

Controls reconcile selected table bytes against larger outer-plus-nested observed
allocation, check explicit and detached copies, inactive rows, moves and domain
lifetime, and verify complete unwind when the first table request is denied.
The full owner witness independently locates every retained table allocation and
checks the exact selected-domain sum. Constructor byte/work boundary sweeps,
allocation-failure traces, unchanged-state/retry and repeated exchanges remain
active. Projection, membership and workbench value/native tests qualify affected
readers as well. Remaining key/recipe/input payload, graph/declaration/catalogue
construction, full setup work, domain overhead and combined borrowed/caller
resources are still open. This is not complete projection or constructor admission.

## Work inventory and remaining budget gate

### Declaration table allocation domain (contribution awaiting review)

Owner declaration construction now receives its shared reservation domain before
building outgoing/incoming source tables or ordered leaves. The temporary aggregate
and parts tables use the same allocator and move their backing into the result.
The aggregate is appended by move instead of copied through an initializer list.
Vector requests, including overlapping old/new growth backing, enter the domain
before allocation. The reverse declaration uses an explicit domain-bound copy;
its three independent backing requests are also admitted before copying rows.
The shared declaration object/control-block request remains a later separate gate.

Ordinary value-copy construction detaches all three table allocations. Moves and
swaps carry the domain with the backing; no domain-to-declaration ownership cycle
exists. Declaration aliases retain the tables through owner destruction. Their
backing is released at pointee destruction; shared control blocks retain their
own charge through final weak release. The source/leaf accessors now return const
allocator-aware vectors and the retained census accepts those container types.

This is table admission, not complete declaration admission. Owner/recipe UUID
copies, separately allocated source/leaf keys, direct-parent vectors and their
nested payload, lookup/matching work, stack and allocator/exception overhead are
still outside this gate. Row temporaries may copy nested payload before a table
request. Embedded row headers are already part of the table allocation and must
not be charged twice. Selected domain and retained census totals overlap.

Construction and explicit-copy controls independently sweep successive byte/work
rejection boundaries, check all observed storage is reclaimed, and retry with
exact allowances. Full structural equivalence includes recipe/occurrence identity,
source definitions/parents/masses, ordered leaves, exposures and match work.
Detached snapshots remain usable after source-domain destruction, while moved
values keep that domain alive. The full owner witness independently locates every
retained declaration-table allocation and reconciles the complete selected-domain
sum. Existing failure-site, setup rejection, shared lifetime and repeated native
exchange/tick controls remain active. Complete declaration/setup accounting and
combined caller/borrowed resources remain open.

<a id="membership-construction-allocation-domain-contribution-awaiting-review"></a>

### Membership construction allocation domain (accepted #246)

Owner catalogue collection now receives the shared reservation domain before it
builds the catalogue. Its retained contribution vector and temporary tool lookup,
visit-state, reached-target and occurrence-owner maps use allocator-aware storage.
Every reached-target set is explicitly constructed with that same allocator;
default insertion of a detached set is not used. Standard-library rebinding
charges actual map/set node requests, including their embedded key/value headers,
without assuming a node layout. Contribution-vector growth includes old/new
backing overlap. Shared-object allocation remains separately admitted afterward.

The graph nodes are released on return or unwind; only the contribution backing
survives in the catalogue. That backing retains the domain through catalogue
aliases and moves. Its charge ends at pointee destruction, while the separate
shared object/control-block charge continues through the last weak reference.
Ordinary value snapshots detach their copied vector backing from the source
quota. Detached standalone collection still uses the same algorithm with no owner
domain. The contribution accessor now exposes an allocator-aware vector by const
reference; in-tree value/native readers are part of the validation scope.

This extends actual allocation admission into preceding catalogue construction,
but does not bound the whole graph traversal: separately allocated UUID/address/
path text and vectors, callable backing, recursion/exception/allocator overhead,
comparisons and other setup work remain outside this gate. Row payload can be
constructed before a contribution-table request. No thread-local interception,
database value-type change or additional native state authority is introduced.
The top-level map headers live on the stack and are not heap charges; nested set
headers are embedded in the admitted reached-map nodes.

Catalogue-only controls sweep every successive byte/work rejection boundary,
reclaim all observed allocations and retry with exact allowances. Detached copies
remain usable after domain death and match standalone membership identities,
targets and counts. The owner witness independently reconciles contribution-table
requests against the allocation observer, and checks table release at last strong
alias followed by control-block release at last weak alias. Existing complete
owner allocation-failure, setup rejection and repeated-operation checks remain.
The selected domain overlaps the retained census; do not add their totals.

<a id="projection-setup-work-gate-contribution-awaiting-review"></a>

### Projection setup work gate (accepted #245 through combined #246 stack)

Owner adoption now requires a separate explicit projection-setup work allowance.
The input gate continues to authorize the initial projection snapshot. This new
meter covers the spare projection copy and exposure-pool initialization of both
retained projections. It does not meter catalogue/declaration construction, map
setup, other validation or the complete constructor.

One admission unit is required before the domain/owner allocation. After the
existing catalogue/declaration checks, the spare projection's logical census uses
the same incremental inspection meter as input admission. Its complete logical
payload charge, including object/table headers and conservative text terminators,
is prepaid before allocating or copying the spare projection. This is structural
copy work, not actual allocation bytes or a memory-peak estimate.

Each exposure-pool initialization charges admission, occurrence visits and tool
identity comparisons before the corresponding reads. Each reserve charges one
dispatch unit plus existing parent-row header bytes if growth will move rows.
Inactive parent creation charges the row header and occurrence/object key payload
before copying. The allocation domain independently gates actual outer-table
requests; nested backing remains outside that allocation gate. Inline/scalar
work is included in the enclosing structural charge, not instruction counting.

`ProjectionSetupResult` identifies Admission, SpareInspection, SpareCopy,
CommittedParents, SpareParents or Complete and reports cumulative reserved work.
Work exhaustion/overflow returns no owner after unwinding all constructed data;
caller inputs and borrowed native execution remain unchanged. Partial parent pools
are discarded, never retried in place. A fresh adoption with sufficient allowance
reconstructs them. Allocation rejection still has its separate reservation result;
input rejection leaves setup in Admission with zero work. Complete is set only
after owner construction succeeds. An Ok work result in an earlier phase does
not mean adoption succeeded.

Both owner fixtures sweep successive work boundaries across every phase, verify
zero-allocation rejection for a zero allowance, complete reclamation, unchanged
native authority/state and exact fresh retry. Existing allocation-failure,
reservation and repeated exchange/tick witnesses remain active. Structural work
and allocation-request work overlap conceptually and are independent allowances;
neither should be added to the retained byte census. Full setup work, nested
allocation limits and combined caller/borrowed accounting remain outstanding.

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
steps now have the separate staging gate described below. The following command gate supplies the later
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

The accepted #235 contribution introduced separate capture and command-work allowances.
At that source, `OwnerPreparationResult` identifies `Capture`, `Commands` or `Complete` and
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

<a id="owner-staging-work-gate-contribution-awaiting-review"></a>

### Owner staging work gate (accepted #237)

The private owner now takes three explicit work allowances: staging, capture and
commands. `OwnerPreparationResult` adds `Staging` and a separate `staging` result.
Stopping there leaves capture and command charges zero; interpret those unused
results with the phase. `Complete` still means preparation only, before sealing
and publication. The pending, consumed, sealed and binding-valid gates are
invalidated before the first charge, including when replacing a pending plan.

| Surface | Structural work units |
|---|---|
| Owner and projection admission, binding staging | One each, before validation or payload changes |
| Declaration lookup | Shared-meter key comparison, then one per selection visited to check current exposure |
| Projection compatibility | Shared-meter UUID/pin comparisons; one per current-exposure lookup row |
| Exposure/active flags and occurrence traversal | One per visited row before reading/updating scalar fields |
| Tool/parent matching | Shared-meter identity comparisons, plus one per immutable parent row |
| Parent swap | Three `sizeof(ParentContribution)` units before swapping retained objects |
| Parent-pool transfer | Two `sizeof(ParentContribution)` units per transferred row, prepaid for the entire move-and-clear/resize block |
| Initial binding maps | Binding object size plus key-copy dispatch and UUID/path/address payload before each row copy |

Parent-pool transfers are indivisible with respect to work exhaustion. Stopping
between a move and removal of the moved-from row would break retry. Charges for
the entire block precede its first move; reserved capacity and nonallocating moves
preserve the existing pool invariant. Exhaustion elsewhere may leave partial
flags, a valid partial parent partition or partial binding maps. Retry overwrites
flags, reunites both parent pools and rebuilds maps. Committed state and native
execution remain untouched. Work depends on retained scratch order and active
pool sizes; `next_required` describes this attempt's next charge, not a guaranteed
whole-operation allowance for a later attempt. Exact-allowance tests use the same
normalized scratch starting state and additionally recover after every stop.

Tests sweep every charge boundary in both single-occurrence directions, recovering
successfully after each rejection. Zero/late-short/exact allowances also cover
repeated single/four-occurrence edits, unchanged completed state and retained
backing, and no observed C++ `new`. Existing numerical and lifetime witnesses stay
active. As with capture/commands, exception-runtime costs are outside that witness.

Standalone/adoption binding preparation explicitly uses maximum staging work
pending setup admission. This gate is structural work, not a memory reservation,
allocator limit or timing claim. Adoption-byte/setup-work gates, caller construction,
combined caller/borrowed resources and remaining-operation integration are still open.
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
| Declaration selection | Staging gate charges retained alternative key comparisons and current exposure lookup |
| Projection staging | Staging gate charges flag scans, key searches, swaps and complete parent-pool transfers before work |
| Binding capture | Incremental structural-work gate before scans, key comparisons, masks and final validation; stage-map copying uses the preceding staging allowance |
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
