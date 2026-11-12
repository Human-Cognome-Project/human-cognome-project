# Private exchange-owner resource inventory

Accepted through #259 on 2026-11-10; [hosted acceptance](../tests/admission/README.md#maintainer-integrated-acceptance--247-through-259-2026-11-10)
records the integrated stack. Remaining limits below are still open.

This inventories the private `assembly::detail::ExchangeOwner` on the explicit
integrated allocation stack through #259. It accompanies the retained-capacity census and scoped
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

<a id="declaration-table-allocation-domain-contribution-awaiting-review"></a>

### Declaration table allocation domain (accepted #247 via #259)

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
outside this #247 gate; the following contribution extends it to direct-parent
vector backing. Row temporaries may copy nested payload before a table
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

<a id="declaration-parent-allocation-domain-contribution-awaiting-review"></a>

### Declaration parent allocation domain (accepted #250 via #259)

Each `ExchangeSource::parents` vector now uses the shared reservation allocator.
Initial declaration construction copies parent rows directly into a domain-bound
vector. Explicit declaration copying reserves each source table, then constructs
every source with an explicit domain-bound parent copy. It does not first make
detached parent copies and attach them afterward. Moves into source tables and
their growth preserve the same parent backing/domain; reverse-side swaps keep it
attached. Ordinary value-copy construction recursively detaches the source and
parent tables; copy assignment retains the destination allocator as before.

The parent-vector allocation is admitted before its row copies. UUID/address
backing within each `warm::Parent`, occurrence/definition keys constructed before
the parent request, other recipe/input parent vectors and setup work remain
outside this gate. Parent object headers are included once in vector backing;
the extra allocator reference is embedded in each source row and thus already
covered by its outer table request/census. This is not a second full-row charge.

Construction and explicit-copy controls now verify every parent allocator and
include parent backing in their exact selected-domain sum, rejection sweeps,
reclamation and exact retry. Detached snapshots check nested allocators and remain
usable after source-domain destruction; moves preserve the original lifetime.
The owner witness independently locates every retained parent allocation alongside
the source/leaf tables. Structural comparisons still include parent target pins,
ordinals, stored mass and contributions; no exchange or awake/asleep behavior is
changed. Remaining key/recipe/input allocation and full setup/combined-resource
qualification remain open.

<a id="declaration-occurrence-path-allocation-domain-contribution-awaiting-review"></a>

### Declaration occurrence-path allocation domain (accepted #252 via #259)

The reservation primitives now live in `common/reservation_allocator.h`, with
engine-private aliases preserving existing names. This is standard-C++ support,
not an engine dependency for warm values or a new execution authority. The move
is a separate mechanical commit; only the namespace changes in the implementation.
`warm::OccurrenceId::path` now uses `warm::OccurrencePath`, an allocator-aware
vector of the same uint32 ordinals. Its C++ container type/layout changes; callers
with concrete std::vector containers must use iterator-range conversion. Identity,
ordering and PostgreSQL text encoding are unchanged. Shared-header changes trigger
both native engine and warm toolbox CI.

Only declaration owner/source paths explicitly attach to the owner domain in
this contribution. Initial collection and explicit reverse copies allocate paths
with that domain before copying ordinals. Aggregate owner paths are empty but
retain the explicit allocator. Source-row moves and reverse swaps carry backing
and domain together. Ordinary value-copy construction detaches; assignment keeps
the destination allocator and moves propagate it. Standalone warm reads and other
recipe/input/projection/map paths stay detached and outside this allocation gate.

The source tables already charge embedded path headers, including the additional
shared allocator reference; ordinal backing is charged separately exactly once.
This container-layout change also changes other retained object/table sizes, even
where their path backing remains detached. The selected domain still overlaps the
whole-owner census and cannot be added to it. UUID/address payload, recipe/input
paths and complete construction/setup work remain open.

Controls reconcile every source-path request with the independent allocation
observer, extend declaration construction/copy budget rejection and exact retries,
and check allocator attachment, detached snapshots, moved lifetime and unchanged
structural values. A server-free warm test exercises copy/move identity and ordering,
exact PostgreSQL parameter text (empty and maximum uint32 paths), and decoding via
a locally constructed libpq result. Warm clients compile without engine linkage.
No schema migration, database writes or activity-state behavior changes occur.

<a id="projection-occurrence-path-allocation-domain-contribution-awaiting-review"></a>

### Projection occurrence-path allocation domain (accepted #253 via #259)

Both retained projections now explicitly attach occurrence identity paths,
optional byte-provenance owner paths, and active/spare parent-contribution paths
to their owner allocation domain. Explicit copies reserve the destination table
and construct each row's path with that domain before copying ordinals; they do
not make detached nested copies first. Inactive-pool initialization uses the same
domain before constructing each contribution. Empty paths retain the allocator
without an ordinal allocation. Table requests already include the embedded path
headers; separate ordinal backing is counted once.

Ordinary value-copy construction still detaches all nested paths, while moves and
pool transfers carry their backing/domain together. Active/inactive exchanges
reuse the reserved contributions without new allocation. Explicit copying of an
already populated spare pool is covered as well as the usual copy-before-reserve
owner setup. No C++ layout, warm encoding, identity, native publication or numerical
behavior changes in this slice; #252's documented value-layout change is inherited.

Projection copy/reservation controls now check all nested allocators, detached
snapshots after source death and moved lifetimes, sweep successive byte/work
rejection boundaries with complete reclamation, and retry at exact measured
allowances. The owner observer independently locates every retained projection
path request in both projections and reconciles it with the selected-domain sum.
Existing failure-site, setup refusal and repeated allocation-free exchange/tick
controls remain active. Reports overlap the retained census and must not be added.

Recipe/input, catalogue contribution and map paths, UUID/address payload and other nested allocations are
still outside this extension. Recipe copying still precedes the first projection
table request. Complete setup and combined caller/borrowed accounting remain open;
this is not a total constructor or machine budget. Deferred activity research is
unchanged.

<a id="catalogue-occurrence-path-allocation-domain-contribution-awaiting-review"></a>

### Catalogue occurrence-path allocation domain (accepted #254 via #259)

`MembershipFields::collect` now constructs each contribution's occurrence path
with the supplied reservation domain before copying ordinals. The finished key
moves into the contribution row; vector growth moves existing rows with their
backing/domain. Empty aggregate paths retain the allocator without an ordinal
request. Existing membership graph traversal, deduplication, pinned targets,
identities and exact contribution values are unchanged. Standalone collection
still receives no domain.

Path backing is separate from the embedded key header already covered by the
contribution table request. A new row's path can be constructed before table
growth; both requests are admitted individually, and rejection unwinds the
pending row and catalogue. Ordinary snapshots detach nested paths and remain
usable after domain destruction. Moves retain their accounting lifetime. Paths
release at last strong catalogue alias, while the shared object/control block
continues through final weak release. No global allocator or execution authority
is added, and counters still require exclusive synchronous access.

Focused catalogue controls now include path capacities and allocator attachment,
detached snapshots and moved lifetimes. Existing successive byte/work rejection
sweeps and exact retries include the nested requests. The independent owner
observer locates each path allocation and includes it in selected-domain and
strong/weak lifetime reconciliation. Membership value comparisons and repeated
exchange/tick controls remain active. These overlapping reports must not be added
to the retained census.

Map and recipe/input paths, UUID/address backing, other setup/traversal work and
combined caller/borrowed accounting remain open. The next bounded slice is
occurrence-map path backing in `ExchangeMap<OccurrenceBinding>` construction,
explicit setup copies and reservation, including inactive slots; field address
backing is a separate remaining item. Deferred wake/sleep research is unchanged.

<a id="occurrence-map-path-allocation-domain-contribution-awaiting-review"></a>

### Occurrence-map path allocation domain (accepted #255 via #259)

`ExchangeMap` now retains a shared reservation-domain handle before its raw outer
allocator. That handle survives empty maps and is destroyed after all slot backing;
whole-map swaps exchange it with the allocator/store. Owner, binding and coordinator
setup carry the shared domain into both map kinds. Field-address backing stays
unattached; only its existing outer table is charged. The added handle is embedded
in each map object and covered by the existing unique-object/census charge.

Initial occurrence-map construction creates destination slots, attaches their
empty paths, then assigns source rows. Explicit setup copies attach before
reserving each source slot's nested capacity, including inactive slots, and copy
values into that capacity. This avoids a detached path copy followed by quota
attachment or a second size-then-capacity allocation. Reservation attaches newly
constructed inactive paths before reserving their ordinal backing. Existing slots
retain their domain when the outer table grows. UUID construction/copying and
field addresses remain outside this gate.

Ordinary map snapshots select no domain and preserve logical rows plus inactive
nested capacity. Assignment retains destination allocators/capacities; erase
rotates complete slots, and publication swaps complete stores and domain handles.
No move constructor is introduced: ordinary copy semantics remain detached.
Preparation and publication continue to reuse prepared storage. Paths and the
outer allocation retain the domain through map destruction, including maps that
were initially empty. Counters still require exclusive synchronous access.

Controls cover initial construction and explicit setup copying, active/inactive
attachment, successive byte/work rejection, allocation-site failure injection,
reclamation and exact retries. Old/new outer-growth overlap and preallocation
refusal checks remain. Detached snapshots survive source-domain death; empty and
populated maps retain the required lifetime. Assignment, append, erase and
cross-domain swap run under the zero-allocation guard. The owner independently
locates every retained path request in its own, binding and candidate maps,
including inactive storage, and reconciles the selected-domain sum. Existing
repeated exchange/tick controls remain active. Domain and census reports overlap.

Next: field-map address backing in `ExchangeMap<ExchangeFieldBinding>`, first
providing engine-independent allocator-aware address storage with unchanged
address identity and codec/database encoding. Recipe/input, other UUID/address
payload, complete setup work and combined caller/borrowed accounting remain open.
No numerical/publication or deferred activity behavior changes here.

<a id="field-map-address-allocation-domain-contribution-awaiting-review"></a>

### Field-map address allocation domain (accepted #256 via #259)

`codec::Address` now uses the neutral shared reservation allocator for its existing
`AddressElement` sequence. The codec remains engine-independent; pure codec
transforms and default values produce detached addresses. Element representation,
identity, validation, ordering, dotted rendering, positional delta/reconstruction
and PostgreSQL literal-pair text-array encoding are unchanged. This is a C++
container type/layout change, not binary compatibility: concrete std::vector
callers use iterator-range conversions. The extra allocator handle is embedded
wherever an address occurs, including values outside the selected domain.

Only `ExchangeFieldBinding` map slots explicitly attach address backing in this
contribution. The same setup construction/copy/reservation hooks used for occurrence
paths attach empty address vectors before allocation. Both active and inactive
slots retain the domain through assignment, growth, erase and whole-map swap.
Ordinary map snapshots recursively detach and preserve inactive capacity. Other
warm/recipe/projection/catalogue/declaration addresses remain detached, as do the
codec's ordinary results. UUID payload and full setup work remain separate gaps.

The map controls now run for both occurrence and field maps, with the actual
nested element size. They retain byte/work rejection, allocation failure,
reclamation, exact retry, growth overlap, detached/empty-map lifetime and no-new
assignment/append/erase/swap checks. Independent owner observation locates every
address request in all three field maps, including inactive slots. Header/table
requests already include each embedded allocator reference; address backing is
counted separately once. These domain and census reports overlap.

A server-free warm test exercises production parameter rendering/decoding via a
locally constructed libpq result, empty/wildcard rejection for object identities,
codec wildcard validation, ordering, delta/reconstruction and detached-copy/move
lifetime. Existing codec unit/adversarial, command IR/span and pure WAL tests
qualify other address consumers. Warm clients and native integration compile;
no live-store writes or schema changes are involved. Repository smoke now watches
`common/**` as well as codec sources, matching the new header dependency.

Next: declaration address backing in source definition refs, parent target refs
and ordered leaf refs during collection and explicit reverse copies, preserving
matching and detached snapshots. Other UUID/address/recipe/input payload, complete
setup and combined caller/borrowed accounting remain open before operation
integration. Deferred activity and GPU sequencing are unchanged.

<a id="declaration-address-allocation-domain-contribution-awaiting-review"></a>

### Declaration address allocation domain (accepted #257 via #259)

Initial `LetterExchangeContract::collect` and explicit domain-bound reverse copies
now construct source-definition, direct-parent-target and ordered-leaf reference
addresses with the supplied domain before allocation. Each parent/leaf is copied
in its existing order with unchanged scope, revision, ordinal, mass/contribution
and byte/nibble identity. Matching rules and exact references are unchanged.
Standalone declarations and ordinary snapshots remain detached; moves retain the
source domain through final address release. No address type or format changes
are introduced by this slice.

The declaration controls include every retained address capacity in the exact
backing sum, alongside tables, paths and parent vectors. Construction and explicit
copy sweeps exercise byte/work refusal, complete reclamation and exact retry;
snapshots survive source-domain destruction and moved values retain attachment.
Independent whole-owner allocation observation locates each address request,
including reverse declarations, and existing allocation-site failure and repeated
native no-new witnesses remain active. Address backing is counted once; embedded
allocator headers already belong to the table requests. The domain, whole-owner
observation and retained census overlap and must not be summed.

Next: retained projection occurrence definitions, optional provenance character/
complete-character refs and active/spare parent field addresses in explicit copies
and parent-pool setup. UUID text, catalogue/recipe/input payload, complete setup
work and combined caller/borrowed resources remain open before operation integration
and same-execution controls. Deferred activity and GPU sequencing are unchanged.

<a id="projection-address-allocation-domain-contribution-awaiting-review"></a>

### Projection address allocation domain (accepted #258 via #259)

Both retained projections now construct occurrence-definition, optional byte-
provenance character and complete-character addresses in their supplied domain.
Explicit copies also attach active/spare parent field addresses; inactive-pool
initialization attaches addresses before copying the pinned parent object.
Scope/revision identity, field kind, ordinals/counts, exposure and pool order are
unchanged. Finished rows move into their tables without detached address copies.
Ordinary snapshots recursively detach; moves and pool transfers retain backing
and domain together. There is no new type/layout, codec or numerical change.

Copy/reservation controls include address capacity in exact backing totals and
check nested attachment, populated spare-pool copies, detached snapshots after
source death and moved lifetimes. Byte/work refusal and exact-retry sweeps cover
both initial copies with inactive-pool setup and copies of an already populated
pool. Structural comparisons preserve every occurrence reference, optional
provenance value and parent row in order. Independent owner observation locates
all retained address requests in both projections; existing allocation-site
failure and repeated native zero-new controls remain active. Address backing is
counted once, alongside tables that already contain allocator headers. Selected,
whole-owner and retained-census reports overlap and must not be summed.

Next: retained catalogue contribution field and pinned-target addresses in
`MembershipFields::collect`. Temporary graph key addresses, UUID text, recipe/
input payload, complete setup work and combined caller/borrowed resources remain
open before remaining-operation integration and same-execution controls. Deferred
activity and GPU sequencing are unchanged.

<a id="catalogue-address-allocation-domain-contribution-awaiting-review"></a>

### Catalogue address allocation domain (accepted #259)

`MembershipFields::collect` now constructs each retained contribution's field
object and pinned target address with the supplied domain before copying elements.
Finished rows move into the contribution table. Exact scope/revision, occurrence,
field kind and count remain unchanged, as do traversal, deduplication and row
order. Ordinary snapshots detach recursively; moves retain backing/domain lifetime.
No new type/layout, codec, schema or numerical change is introduced.

Catalogue controls include both addresses in exact retained backing totals, check
attachment/detachment and moved lifetime, and compare exact ordered rows after
source-domain destruction and exact retry. Existing byte/work refusal sweeps and
whole-owner allocation-site injection cover the new requests. Independent owner
observation locates each retained address allocation. Strong aliases preserve the
backing after owner death; final strong release reclaims all table/path/address
storage while final weak release reclaims the shared control block. Repeated native
zero-new checks remain active. Embedded allocator headers belong to table requests;
address backing is counted once. Domain, whole-owner and census totals overlap.

Next: temporary catalogue `tools`/`state`/`reached` map-key and `Targets` set-element
addresses, including ancestor copies. Node allocation limits alone do not cover
those nested addresses. UUID text, recipe/input payload, complete setup work and
combined caller/borrowed resources remain open before operation integration and
same-execution controls. Deferred activity and GPU sequencing are unchanged.

<a id="catalogue-graph-address-allocation-domain-contribution-awaiting-review"></a>

### Catalogue graph-address allocation domain (provisionally accepted #261)

Temporary catalogue `tools`, `state` and `reached` map keys and nested `Targets`
set elements now construct addresses in the supplied domain before insertion.
State/reached entries and target elements are looked up before constructing a
missing key, avoiding detached ancestor-range copies and allocations for known
targets. Tool insertion retains its existing first-value behavior for duplicate
keys; rejected temporary keys release their backing. Exact graph identity,
traversal, ancestor deduplication and cycle rejection remain unchanged.

The diamond fixture has five-element addresses whose 15-byte requests are distinct
from its UUID text, path, table/node and callable allocations. Optional nonthrowing
callbacks in the test allocation witness independently count those requests and
releases, compare their prepayment against the domain, and verify the expected
transient-plus-retained address peak. Four visited vertices and ancestor-set
sizes 3+1+1+0 provide a fixture-derived temporary count; direct and mediated paths
to the same ancestor must deduplicate. This is a fixture-specific C++ allocation
witness, not a production allocator-size assumption or total machine budget.

The same fixture runs byte/work refusal and exact retry, every observed allocation
failure, post-failure retry, duplicate-tool insertion and cycle-rejection cleanup.
Existing owner retained-request, shared lifetime and native zero-new checks remain
active. Temporary addresses release before collection returns and are not added
to the retained census. Request work, peak overlap and retained totals are distinct.

Next: addresses throughout both retained projection recipes (tool/parent/member
refs, geometry, occurrence definitions and focal fields), attached during explicit
copies before allocation. Recipe paths/tables, UUID/text/input payload, complete
setup and combined caller/borrowed accounting remain open before operation/control
integration. Deferred activity and GPU sequencing are unchanged.

<a id="recipe-address-allocation-domain-contribution-awaiting-review"></a>

### Recipe address allocation domain (provisionally accepted #262)

Explicit owner copies of `LetterProjection` now build `recipe_` addressed rows
with the supplied domain: tool references, parent targets, member-of references,
geometry objects, legacy/structural occurrence definitions and focal fields.
Each address is admitted before copying elements. A detached whole-recipe copy
is not made first. Finished rows move into reserved ordinary recipe vectors;
those vectors and other recipe payload remain outside this address-only gate.

The copy preserves every Bundle field: publication/owner/creator pins, version,
tool metadata, optional values, ordered parents/memberships, geometry/spheres,
nodes, occurrence identity/state, selections, contacts and focal fields. Ordinary
snapshots detach addresses recursively; moves retain their domain. There is no
new type/layout, codec, schema or numerical change.

Projection controls now include recipe addresses in exact backing totals and
independently locate retained requests. A nonallocating comparison checks all
recipe values alongside projection rows. Existing structural and populated-pool
controls are joined by a legacy fixture with nonzero positions/velocities,
geometry and contacts. Both versions exercise byte/work refusal, cleanup, exact
retry, detached snapshots after source death and moved lifetime. Whole-owner
allocation failures and repeated native zero-new checks remain active. Embedded
headers belong to their containing rows; backing is counted once and overlapping
selected/whole-owner/census reports must not be summed.

Next: recipe occurrence paths in legacy/structural rows, selection parts and
contact endpoints. Recipe tables, UUID/text/input payload, complete setup work
and combined caller/borrowed accounting remain open before operation/control
integration. Deferred activity and GPU sequencing are unchanged.

<a id="recipe-path-allocation-domain-contribution-awaiting-review"></a>
### Recipe path allocation domain (provisionally accepted #263)

Explicit owner recipe copies now construct legacy/structural occurrence ID paths,
every selection part path and both contact endpoint paths in the supplied domain
before copying ordinals. The shared projection-ID helper preserves entry numbers;
finished IDs and rows move into their tables. Empty aggregate paths retain the
domain without allocating ordinal storage. Existing recipe addresses remain
attached; no detached recipe/path copy precedes admission.

Projection path controls now include every recipe path in exact backing totals,
allocator attachment/detachment, source-death snapshots and moved lifetime.
Independent allocation observation checks the actual requests. Existing complete
recipe comparisons preserve IDs, order, contacts, metadata and other values.
Validated legacy and structural fixtures cover empty aggregate endpoints and
contacts with nonempty paths at both ends, alongside selection/occurrence paths.
Both run refusal/reclamation/exact-retry controls; populated spare-pool, whole-owner
allocation failures and repeated native zero-new checks remain active. No type/
layout, codec, schema or numerical change is introduced. Reports overlap the
retained census and must not be summed.

Next: the eight top-level Bundle table stores, using engine-independent allocator
support and explicitly qualified C++ type/layout compatibility. Nested recipe
tables, UUID/text/input payload, complete setup and combined caller/borrowed
resources remain open before operation/control integration. Deferred activity
and GPU sequencing are unchanged.

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

## Integrated stack acceptance — 2026-11-10

Integrated at `d85213d2744a9ad4bb7cb733d7ad61dfef7e1ea9` through #259, reviewed head `a2636cad6e85b9582f7e37aa1dd537bbd7b5c430`. Native 42874078745 and smoke 42874078677 passed; all 12 native jobs succeeded. Inspected regression shards passed 29 + 29 + 29 + 28 = 115 tests; database log reports addressed_reads=24/private_reads=0.

The scoped allocation gates above are accepted; this does not close complete owner resources, setup, operations or hardware qualification. Earlier provisional evidence remains historical.

### Recipe table allocation domain (contribution awaiting review)

The eight top-level retained Bundle stores (tools, geometry, nodes, legacy
occurrences, selections, contacts, structural occurrences and focal fields) use
`warm::RecipeTable<T>`, an alias of the existing engine-independent shared
reservation vector. Explicit owner projection copies select the destination
allocator before reserve, construct each row with existing nested address/path
attachment, and move finished rows into tables. Empty tables retain the domain
without allocating elements. Ordinary copy construction detaches recursively;
moves keep backing and domain alive. This remains exclusive synchronous accounting.

This changes C++ container types and Bundle layout, requiring consumers to rebuild.
On the pinned Clang 15/libstdc++ x64 build, table headers grow from 24 to 40 bytes;
Bundle grows from 328 to 456 bytes (+128). The eight vector headers each gain
shared allocator state; no stable C++ ABI is
claimed. Ordinary-vector assignments use explicit iterator conversion in the
legacy focal adapter and PostgreSQL fixture. Prepared contacts use RecipeTable;
the structural adapter test converts its focal table explicitly. The public
adapter still accepts std::vector<FocalField>. Serialization, SQL, recipe versions,
values and numerical behavior are unchanged. Standalone storage stays detached.

The table visitor now includes all eight stores in exact selected backing and
independent allocation-request checks for both owner projections. Existing v1/v2
copy controls cover all domain charge boundaries, reclamation and exact retry,
complete recipe equivalence, detached snapshots after source death, moved lifetime,
populated inactive pools and whole-owner allocation-failure injection. Empty
version-specific tables also participate in allocator attachment checks.

Nested Tool.parents/member_of, Geometry.spheres and Selection.parts table backing
remains outside this domain and is the next slice. Their addresses/paths are already
attached. UUID/text/input payload, preceding setup, allocator metadata and combined
resources remain separate. Selected reports overlap the retained census; do not
add them together. See source-pinned admission evidence for qualification limits.
<a id="nested-recipe-table-allocation-domain-contribution-awaiting-review"></a>
### Nested recipe table allocation domain (provisionally accepted #265)

Tool.parents/member_of, Geometry.spheres and Selection.parts now use the same
engine-independent RecipeTable storage as the eight top-level Bundle tables.
Explicit retained projection copies select the destination domain before reserve,
then move complete rows with attached addresses/paths. Spheres are copied directly
into attached storage. Empty nested tables keep the domain without element
allocation. Standalone copies detach recursively; moves retain backing/domain.

The table visitor traverses top-level and nested stores without allocating test
metadata. Both owner projections and validated legacy/structural copy controls
include independent actual requests, exact selected backing, allocator attachment,
snapshots after source death, moved lifetime, charge-boundary refusal/reclamation/
exact retry, populated inactive pools and whole-owner allocation failures.
The legacy validator's expected-parts temporary uses detached RecipeTable for
same-type exact comparison; validation semantics and encoding stay unchanged.

On the pinned Clang 15/libstdc++ x64 toolchain, Tool grows 304 -> 336 bytes,
Geometry 120 -> 136 and Selection 40 -> 56; Bundle remains 456 bytes.
C++ nested table types and Tool/Geometry/Selection layouts change, requiring all
consumers to rebuild. This is not a stable-ABI claim. Domain headers and element
requests remain distinct from allocator metadata/physical residency. Existing
census code already visits nested stores and sizeof rows; selected totals overlap
that census. No new numerical authority or runtime algorithm is introduced.

Next: retained Tool.recipe and optional Tool.notation text, including exact bytes,
terminators, short/long-string behavior, detached snapshots and before-allocation
attachment. UUID/input payload, full setup and combined resources remain open.
See admission evidence for source pins, compatibility/build results and limits.
<a id="recipe-text-allocation-domain-contribution-awaiting-review"></a>
### Recipe text allocation domain (provisionally accepted #266)

Tool.recipe and optional Tool.notation use RecipeText, a basic_string with the
existing engine-independent shared reservation allocator. Explicit owner copies
construct it with the destination allocator before character allocation. Empty
optional state remains absent; empty/short strings retain the allocator without
heap characters. Ordinary copies detach; moves keep domain/backing alive.

Actual heap requests include capacity plus one terminating byte. Inline characters
are already within the string/Tool object; recipe_text_backing detects them by
whether data lies within that object, not a hard-coded short-string length.
The retained census uses heap backing only for these fields. Logical-copy input
preflight excludes strings fitting the default inline capacity, and otherwise
charges size plus terminator; this is the payload-copy admission witness, not a
new claim that all allocator overhead or global input payload is complete. Existing
non-recipe UUID/input std::string accounting remains separate and unchanged.

On pinned Clang 15/libstdc++ x64, string/optional string headers grow from 32/40 to
48/56 bytes, Tool from 336 to 368; Bundle remains 456. Default inline capacity is
15 in that build; a 131-character value requests 132 bytes. Consumers must rebuild;
no stable ABI or other-library layout claim is made. Warm validation uses string
views. SQL parameters, retained-data fixtures and viewport labels/capture use
explicit standard-string/optional conversions, preserving NULL versus empty and
exact bytes. Canonical tool_body encoding and numerical behavior stay unchanged.

Selected-backing and domain visitors now cover text as well as table storage.
Independent request observation checks capacity plus terminator and zero separate
SSO backing. v1/v2 fixtures add valid unreferenced tools with absent, empty, escaped
short and long notation/recipe text without changing requested letter semantics.
Complete recipe and immutable-body comparisons, all charge-boundary refusal/
reclamation/exact-retry, snapshots after source death, moves, populated pools and
whole-owner failure injection remain active. Standalone text tests qualify exact
encoding, optional states, short/long requests and lifetime. Affected viewport
snapshot coverage and warm client builds remain scoped, without live database I/O.

Next: retained recipe UUID payload, with a shared Uuid interface/consumer audit.
Other UUID/input payload, complete setup and combined resources remain open.
Selected allocation reports overlap the census; do not sum them. See admission
evidence for exact source pins, failures, checks and observed resource changes.
### Recipe UUID allocation domain (contribution awaiting review)

Uuid stores UuidText, an engine-independent basic_string with the existing shared
reservation allocator. Its string_view constructor immediately copies and applies
the same length, separator, hexadecimal validation and lowercase normalization.
str() now returns a const UuidText reference instead of const std::string&. Explicit
standard-string conversions adapt SQL parameters, key formatting and viewport JSON;
encoding/comparison/nil semantics remain unchanged. This is a C++ interface/layout
change requiring consumer rebuilds, not a stable ABI promise.

An explicit Uuid(source, domain) copy selects the allocator before copying the
already normalized source payload. Recipe construction uses it for Bundle selection/
revision/owner/creator, Tool reference scopes/revisions/epochs/authors, parent/member
references, Geometry scopes, both occurrence-version scopes and focal-field scopes.
Each UUID's payload includes its terminating byte. Ordinary copy construction
detaches; moves retain the domain. Other projection UUIDs intentionally still use
ordinary detached copies, as do declaration/catalogue/map/other owner UUID payloads.
The shared representation enlarges their headers too; that is not payload admission.

On pinned Clang 15/libstdc++ x64: Uuid is 48 bytes (formerly 32), ObjectKey 88,
ToolRef 136, Tool 432, Parent 168, Geometry 152 and Bundle 520. A normalized
36-character UUID requests 37 bytes. Existing sizeof-based censuses and domain
requests reflect containing-row growth. The text visitor now includes every
recipe UUID in independent actual-request, terminator, allocator attachment,
complete copy/encoding, source-death snapshot, move, all refusal/reclamation/exact
retry and whole-owner failure controls. The standalone UUID test also checks
normalization, malformed values, ordering, nil/non-nil state and SQL parameter
encoding without a live database.

Next: remaining retained projection UUIDs, followed by declaration/catalogue/map/
other owner UUIDs and input payload. Complete setup and combined resources remain
open before operation/control integration. Selected reports overlap the census;
heap request observations do not qualify allocator metadata, physical residency
or performance. Source-pinned scope/results are in admission evidence.

### Integration acceptance through #267 (2026-11-11)

The reviewed #261–#267 allocation contributions are integrated at
`4c0c3c56ea83c7868a62c7dd41de10d183053ec5`, preserving #260. This supersedes
provisional/awaiting-review status labels for those contributions above.
Review 5858105951 accepts the recipe UUID scope at
`7d9a7e484d923d17edc223d95007171b273c2680`; native, warm, viewport and smoke
workflows passed. Scope limits and historical evidence remain unchanged.
Next: remaining retained projection UUIDs; other owner/input UUID payload,
full setup and combined resources remain open before operation/control integration.

### Remaining projection UUID domain (contribution awaiting review)

Explicit LetterProjection copies now attach study identity, occurrence and byte
provenance owner-study IDs, occurrence/byte/complete-character reference scopes
and revisions, and active/spare parent contribution study/field scopes. The
existing projection key/object/reference helpers always select the destination
UUID allocator before copying; reserve_exposure uses those same helpers when
populating inactive contributions. The recipe-only switch is no longer needed.
Existing recipe, table, address, path and text attachment is preserved.

The nonallocating text/backing visitor includes each of these UUID fields,
feeding independent actual requests (including terminators), domain identity,
exact selected live bytes, budget refusal/reclamation/exact retry and detached/
moved lifetime checks. Populated spare-pool copies also verify every backing
allocator points to the new domain. Existing owner operation tests exercise
repeated active/spare transfers without replacing retained UUID storage.

This changes allocation-domain selection, not Uuid representation, projection
values, encoding or numerical behavior. The prior census already counted these
UUID characters; selected accounting overlaps that census. Declaration,
catalogue/map/other owner UUIDs and input payload remain outside this slice.
Next bounded scope is retained declaration UUID payload. Complete setup and
combined resource accounting remain open before operation/control integration.

### Declaration UUID domain (contribution awaiting review)

LetterExchangeContract collection now constructs owner and recipe identities
with their destination allocator directly, eliminating detached default UUID
payloads that were subsequently replaced. Explicit copies attach those same
identities before allocation. Source occurrence study IDs, source/parent/leaf
reference scopes and revisions use explicit Uuid(source, domain) construction.
The public detached declaration path and exact matching semantics are preserved.

A nonallocating UUID visitor extends independent request/terminator, domain,
complete retained-byte, detached snapshot and moved/source-death controls for
both collection and explicit copying. Zero byte/work allowance must refuse
before any observed allocation attempt. Existing boundary sweeps qualify
reclamation and exact retry; whole-owner observation and allocation-failure
injection include every retained declaration UUID. No representation, encoding
or numerical policy changes. Selected totals overlap the retained census.

Next: catalogue UUIDs, auditing retained and temporary graph requests separately.
Map/other owner UUIDs, input payload, complete setup, combined resources and
operation/control integration remain open. #268 is provisionally accepted;
its post-PASS cache warning remains preserved, and integrated acceptance is
through #267 pending hosted reconciliation.

### Retained catalogue UUID domain (contribution awaiting review)

MembershipFields now initializes study/recipe UUIDs directly with the destination
domain. Every retained contribution attaches occurrence study, field scope,
target scope and target revision before copying character payload. The shared
object-copy helper explicitly selects this only for retained rows; UUIDs in
temporary tools/state/reached/ancestor-target graph keys remain detached. Their
existing domain-selected address and node backing is preserved.

A nonallocating retained UUID visitor covers independent request/terminator and
allocator observations, exact retained backing equality, ordinary detached copies,
source-death snapshots, moves, budget refusal/reclamation/exact retry and whole-
owner failure injection. Zero byte/work allowance refuses before allocation.
Catalogue equivalence also compares recipe identity and the alternatives flag.
Existing diamond, duplicate-key and cycle controls keep graph behavior checked.

No representation, encoding or numerical policy changes. Selected reports overlap
the census. Next: temporary graph-key UUIDs with independent transient/retained
request witnesses. Map/other owner UUIDs, input payload, complete setup, combined
resources and operation/control integration remain separate. #268/#269 are
provisionally accepted; integrated acceptance remains through #267.

### Temporary catalogue graph UUID domain (contribution awaiting review)

The catalogue object helper now attaches every scope UUID before allocation,
including keys in tools/state/reached maps and ancestor-target sets. The retained-
only switch is removed. Existing retained UUID, address and node attachment and
graph traversal/deduplication semantics are preserved. UUID representation and
encoding are unchanged. These temporary payloads are charged while live and
released after collection; their cumulative work remains charged.

The pinned graph fixture identifies 15-byte address requests. UUID payload and
comparison formatting both request 37 bytes: the observer tracks prepaid UUID
blocks by pointer, checks their counts against independent graph cardinalities,
and reports unselected same-sized requests separately. A detached 37-byte negative
control qualifies that distinction. Separate counters check acquired/released
bytes, transient/retained overlap and retained-only bytes after graph teardown.
Its diamond has four visited vertices and five unique ancestor-set entries.
Duplicate insertion checks the extra rejected UUID/address and exact reclamation.
Cycle rejection checks prepayment/cleanup, then repairing the cycle must permit
an exact-budget retry. Existing all-site failure injection, full byte/work
refusal sweeps and detached/moved catalogue lifetime controls remain active.

Next: retained ExchangeMap slot UUIDs, including setup/default construction and
inactive-slot reuse. Other owner identities, input payload, full setup and combined
accounting remain separate before operation/control integration. Selected reports
overlap the census; no complete allocator, physical-memory or performance claim.
#268–#270 are provisional; integrated acceptance remains through #267.

Witness calibration exposed same-sized temporary allocations in ObjectKey ordering:
operator< renders warm::key strings, including 37-byte UUID text copies. These
comparison-formatting requests remain unselected setup payload, not graph-key
UUID admission. The initial size-only witness failure is retained in admission
evidence; no comparator behavior was changed to satisfy the new witness.

### Map-slot UUID domain (contribution awaiting review)

A nil Uuid constructor accepts the reservation domain and allocates its normalized
36-character payload directly with that allocator. ExchangeMap reserves outer
backing first, then constructs occurrence/field binding slots with this nil UUID
and domain-selected empty path/address storage. It no longer default-resizes
UUID-bearing rows and attaches allocators afterward. Explicit copies preserve
inactive nested capacities; assignment keeps the destination allocator. Ordinary
snapshots detach. Clear/erase/reuse/swap retain the existing no-allocation behavior.

Every constructed slot's study/scope UUID is included in standalone and owner/
coordinator visitors, independent actual-request/terminator checks, exact domain
backing, refusal/reclamation/retry and all-site failure injection. A budget that
admits only the outer row must refuse the first UUID with exactly one observed
allocation attempt. Active and inactive allocators are checked during assignment,
erase/swap, snapshot source death and empty-map later reservation. The standalone
warm test covers domain-selected nil values, refusal and exact retry.

UUID layout/encoding is unchanged. Slot growth now explicitly reserves the
requested outer capacity before append; measured capacity/request changes must
be reported rather than assumed identical. Selected accounting overlaps the
capacity census. Next: retained projection input top-level tables, after shared
interface/consumer audit. Nested input payload, comparison formatting, other setup,
combined resources and operation/control integration remain separate.

### Retained projection input table domain (contribution awaiting review)

StudyInput and StructuralStudyInput use the shared reservation allocator for their
seven top-level tables: nodes, definitions, occurrences, selections, contacts,
memberships and focal fields. Explicit LetterProjection owner copies construct
these tables with the destination domain before row allocation. Nested std::vector
and std::string payload remains outside this bounded slice. Ordinary value copies
detach through allocator copy selection; moves retain domain lifetime.

The shared-interface audit found concrete std::vector handoffs in public bind
nodes and active-workbench focus; these use iterator assignment into detached
input tables. Native scale binding consumes the new table type. Assembly test
helpers follow that type. Model values and numerical semantics are unchanged;
container layouts grow by allocator state, requiring affected consumer rebuilds.
The existing capacity census already visits these tables and is not added to the
selected-domain report.

The projection visitor includes all fourteen tables, including empty allocators.
Independent pointer/request observations and exact selected-byte equality cover
retained backing. Complete v1/v2 comparisons include capacity, layout pointers,
geometry, parent ordinals/counts, occurrences/starts, selections, contacts,
memberships and focus. Real v1/v2 fixtures supplement nodes/memberships/focus
normally populated at public handoff, exercising copy, snapshot/source death,
move lifetime, byte/work refusal and exact retry. Whole-owner failure injection
and affected native/viewport consumer regressions remain required.

Next: nested input arrays (Definition components/parents and Selection parts),
then input text and remaining setup payload. Full setup, combined accounting and
operation/control integration remain separate. No physical-memory/performance claim.

### Nested retained input array domain (contribution awaiting review)

Definition components/parents and Selection parts now use InputTable. Explicit
projection input copies reserve outer backing, then construct each row with its
nested destination allocator selected before allocation. Other rows use ordinary
value copies. Input text stays detached. Ordinary snapshots detach nested tables;
moves preserve their domains. Empty nested allocators are attached too.

The shared geometry calculation is factored into an allocator-generic internal
helper, preserving the public std::vector interface and its initializer-list
callers without temporary geometry copies. Active-workbench local geometry uses
iterator construction at its public handoff. Assembly fixtures use InputTable.
All configured consumers must rebuild because Definition/Selection layouts grow.

The existing input-table visitor now includes components, parents and parts,
feeding independent pointer/request checks, exact selected accounting and domain
identity assertions. Complete v1/v2 equality, refusal/reclamation/exact retry,
detached source-death snapshots, moved lifetime and whole-owner all-site failure
injection exercise the nested backing. Geometry/order/counts/exposure are unchanged.

Next: retained input text. Other identities/comparison formatting, full setup,
combined resources and operation/control integration remain separately inventoried.
Selected accounting overlaps the capacity census; no physical-memory or performance
qualification follows from these requested-storage observations.


### Integrated acceptance through #274 and pending text work (2026-11-11)

**Queue reconciliation — 2026-11-11:** Reviewed #268–#274 are integrated into main at `b05db31c2e591f8fbcd5c14dc7ddff14ef46b2e9`, reviewed tip `ce3642ee463beb76948a2497c9aeb50172c3267d`. Every head in that stack has successful native and smoke workflows; #272 warm also passed. #274 native 43030713058 passed all 12 jobs; inspected regression logs total 115 tests and database reports addressed_reads=24/private_reads=0. Earlier provisional/pending labels below are historical and superseded by this acceptance.

**Current review queue:** #275 parent-value text, #276 contact/membership/focus text and #277 selection text are source-reviewed and provisionally accepted, but remain unmerged. Inspected native jobs 130564239034 (#275), 130638929459 (#276 retry) and 130648619590 (#277) failed during toolchain installation because azure.archive.ubuntu.com could not resolve, before product compilation. #276 viewport retry job 130638930090 was cancelled during dependency installation with the same DNS failures. Preserve these attempts; complete successful hosted qualification before integration. The next coding slice is definition/occurrence identities and references, per #277 review 5863228338. Other identities/comparison formatting, complete setup, combined accounting and operation/control integration remain open. #75, #111 and #123 remain deliberately held.


### Retained input parent-value text domain (contribution awaiting review)

The authorized input-text workstream is split at shared consumer boundaries.
ParentOccurrence.value now uses InputText, the neutral shared-allocator string
representation. Explicit projection copies construct parent text directly with
the destination allocator, before character allocation, retaining ordinal/count.
Parent tables keep their existing domain attachment. Other input strings remain
outside this bounded slice.

The independent text visitor includes parent values in both input variants.
Existing pointer/request observations distinguish inline storage from heap backing,
check capacity plus terminator and allocator identity, and feed exact selected-byte
accounting. Supplemented v1/v2 fixtures add empty, short and 137-character parent
values to exercise inline/heap copies, detached source-death snapshots, moved
lifetime, byte/work refusal, reclamation and exact retry. Whole-owner all-site
failure injection remains active. Supplementary values are copy fixtures, not
claims of valid native model inputs.

Projection construction explicitly converts encoded keys into InputText. Native
preparation explicitly converts parent values at the existing public group and
participation outputs; no public output encoding or numerical change is intended.
The capacity census uses the same inline-aware backing rule as recipe text,
avoiding a second charge for inline characters. ParentOccurrence layout grows;
affected consumers must rebuild. Selected accounting overlaps the census.

Next: remaining input text and its lookup/serialization boundaries. Other owner
identities/comparison formatting, full setup, combined accounting and operations
remain separate. This slice does not complete input-text or owner resource coverage.

### Retained relation/focus input text domain (contribution awaiting review)

Contact.a/b, Membership.occurrence/group and FocalField.value use InputText.
Explicit projection row construction selects the destination allocator before
character copying. Ordinary copies detach; moves retain domain lifetime. Other
input identities and selection text remain excluded. The shared input-text census
uses inline-aware backing, including heap terminators without double-counting
inline characters in row headers.

Projection and membership binding, active-workbench context, native preparation,
fixture builders and viewport focus callers explicitly adapt allocator boundaries.
Native lookup/output containers remain ordinary strings. These temporary boundary
conversions are not claimed as selected retained input backing or full setup work.
Encoding and numerical policy are unchanged; affected row layouts require rebuilds.

The text visitor covers both fields of contacts/memberships and focal values in
both input variants. Supplemented copy fixtures include empty, short and long
values in every added field; these are copying probes, not valid native models.
Independent pointer/request/terminator/domain checks, exact selected accounting,
complete v1/v2 equality, refusal/reclamation/exact retry, detached source-death
snapshots, moved lifetime and whole-owner failure controls cover the new backing.

Next: definition/occurrence identities and selection text across their shared
lookup/serialization consumers. Other owner identities/comparison formatting, full
setup, combined accounting and operation/control integration remain separate.
Selected reports overlap the census and are not physical-memory/performance bounds.

### Retained selection input text domain (contribution awaiting review)

Selection.aggregate and each Selection.parts element use InputText. Explicit
projection copies reserve the part table with its destination allocator, construct
each string with that allocator before copying characters, and construct the
aggregate text directly in the same domain. Ordinary snapshots detach recursively;
moves retain lifetime. Definition/occurrence identities remain the next slice.

Projection output construction and native selection lookup use explicit conversions,
preserving existing string lookup/output interfaces and values. The review-requested
membership emptiness check now inspects the string directly without allocating.
Other temporary boundary conversions remain unselected setup work.

The shared text visitor includes aggregate and every part in both input variants.
Supplemented empty/short/139-character copy fixtures exercise each new field;
these are copy probes, not native model-validity claims. Independent request,
terminator, inline/heap and domain observations feed exact selected accounting,
complete equality, refusal/reclamation/exact retry, detached source-death snapshots,
moved lifetime and owner failure controls. Selected reports overlap the census.

Next: definition/occurrence identities. Other owner identities/comparison formatting,
full setup, combined resources and operation/control integration remain separate.

### Retained input identity text domain (contribution awaiting review)

Definition.id and both identity/reference fields of Occurrence and
StructuralOccurrence now use InputText. Explicit projection copies construct
these strings with the destination domain before allocating character backing.
This completes retained text attachment in StudyInput and StructuralStudyInput,
including their previously attached parent, selection, relation and focus text.
Ordinary copies detach recursively; moves retain domain lifetime.

Preparation lookup and public participation/group outputs keep their ordinary
string interfaces through explicit boundary conversions. Active binding and
viewport serialization preserve exact values. These temporary conversions remain
unselected setup work; completion of retained input text does not qualify every
owner identity, comparison/formatting allocation or constructor/setup cost.
The shared inline-aware census counts heap capacity plus terminator without
charging inline characters twice. Row layouts change and consumers must rebuild.

The independent text visitor now includes definition IDs and occurrence IDs and
references in both variants. Supplemented empty, short and 139-character copy
fixtures exercise every new field; these are copy probes, not valid model inputs.
Existing pointer/request/domain checks, exact selected accounting, complete v1/v2
equality, refusal/reclamation/exact retry, detached source-death snapshots, moved
lifetime and whole-owner failure injection cover the added backing. The projection
copy witness now requires selected bytes to equal all independently observed
retained allocations, rather than requiring some retained payload to stay excluded.

Next: inventory remaining owner identities/comparison formatting and setup work,
then combined resource accounting and operation/control integration. Selected
accounting overlaps the capacity census; neither is a physical-memory or
performance qualification.
The next audit starts with ObjectKey comparison in kernels/database/warm/values.cpp
(currently formatting keys) and MembershipFields::collect graph traversal/callable
costs. Their existing semantics and accepted ordering must be preserved; retained
input attachment alone does not bound these temporary requests.

### Remaining identity/comparison and setup-cost audit (2026-11-11)

Audited combined source `a75e34084bed5fcfc24280b54111081d6440d58d`, which merges
accepted main `6067c641` into the provisionally reviewed #275–#277/#279 stack.
This is an inventory and sequencing result, not a new complete setup budget.
The accepted mirror repair is present; historical failed/cancelled evidence stays
intact. Earlier text-slice “next” labels above describe their original scope.

| Surface | Owning source and finding | Remaining boundary |
|---|---|---|
| ObjectKey ordering | `kernels/database/warm/values.cpp`, `operator<`, calls `key` twice. Membership tools/state/reached maps and target sets use it. Each comparison formats UUID/address strings outside the selected domain. | Remove formatting while preserving encoded-byte ordering and address validation, then qualify graph and owner callers. |
| Containment and nil checks | `contains` formats both keys only to validate them; `Uuid::nil` constructs a default UUID. Projection/workbench validation uses nil checks; not every such call is on the adoption constructor path. | Separate nonallocating validation from rendering; preserve malformed-input rejection and nil semantics. |
| Adoption identity/argument copies | `ExchangeOwner` passes `{projection_->study(), ...}` to declaration collection; ordinary UUID copy detaches. Owner and validating ExchangeBindings constructors take ordinary row/field vectors by value before building selected maps. | Inventory transient copies and their overlap explicitly. Input-admission logical byte/copy gates do not make these actual requests selected-domain allocations. Preserve caller ownership and detached snapshots. |
| Membership graph traversal | `MembershipFields::collect` uses a recursive `std::function` and loops over tools, references and transitive target sets. Map/set nodes and key backing are already selected. | Callable storage is implementation-dependent; recursion consumes stack. Bound traversal/comparison work and depth independently of heap accounting. Removing type erasure alone would not bound recursion. |
| Declaration and binding setup | `LetterExchangeContract::collect` searches selections/occurrences/tools and assembles declarations; `match_work_` bounds later matching, not collection. Adoption binding calls `prepare(..., UINT64_MAX)`. | Add a constructor/setup work boundary before these traversals; do not reuse later-operation matching/capture claims as constructor evidence. |
| Conversion/output and fixed overhead | `study.cpp`, active binding and viewport deliberately convert to public ordinary strings. `ExchangeOwner::adopt` creates the reservation domain/control block outside its own quota. | Keep output conversion and caller/native/shared ownership separate in the combined inventory; count overlap once and preserve the documented overhead exclusion. |

A local diagnostic linked against this source's `libwarm_values.a` measured C++
new requests on Clang 15/libstdc++; inputs were built before observation:

| Operation | Allocation calls | Cumulative requested bytes |
|---|---:|---:|
| ObjectKey equality, distinct one-element addresses | 0 | 0 |
| ObjectKey ordering, one-element addresses | 4 | 220 |
| Containment, distinct one-element addresses | 4 | 220 |
| Nil check | 1 | 37 |
| Ordinary UUID temporary copy | 1 | 37 |
| Ordering with a 128-element right address | 9 | 1155 |

These are transient request totals, not retained bytes, RSS or timings. The audit
helper and commands are retained in the combined-head evidence archive. Its first
link command incorrectly named a separate codec archive; the codec is already in
warm_values. That helper-only diagnostic is retained; the corrected probe passed
without a product-source change. No callable-allocation size or stack bound was
measured by this probe.

**Next concrete implementation:** remove ObjectKey comparison formatting first.
Preserve `key(a) < key(b)` as an independent ordering oracle: canonical UUID text,
Base62 byte order, both couplet positions, strict prefixes and long addresses.
Validate both addresses even when scopes differ; empty, invalid-element and
partial-tail addresses currently throw. Do not silently convert them into sortable
keys or change the wire encoding. Verify zero C++ new for valid comparisons, exact
ordering/equality properties, malformed-input behavior and unchanged owner/graph
semantics. Nil/containment and temporary-copy changes can follow as separate
bounded slices. Then meter membership/declaration/binding setup, address callable
and stack costs, and reconcile combined resources before operation integration.
### Allocation-free ObjectKey ordering (contribution awaiting review)

Following #279 review 5865153858, ObjectKey ordering validates both addresses
before inspecting scopes, then compares canonical scope text and full couplet
indices lexicographically. Fixed-width full couplets and the byte-ordered Base62
alphabet preserve the `key(a) < key(b)` ordering, including strict prefixes.
Empty, invalid-element and partial addresses still throw the same warm::Error,
even when the scopes alone would decide the ordering. Equality, rendering,
containment and UUID nil/copy behavior are unchanged.

The standalone ordering test compares against the unchanged rendered-key oracle,
checks every adjacent couplet transition and reflexive case, and crosses three
scopes, address lengths 1/2/5/128 and alphabet-boundary values. It independently
observes zero C++ new calls for each valid comparison. Invalid left and right
operands cover empty addresses, invalid first/second indices and partial tails
or non-tail partials across all scope/length fixtures. A negative control links
the new test to the previous implementation and confirms it rejects allocating
comparison. Invalid-input exception allocation is not claimed to be zero.

This eliminates temporary comparison rendering in owner membership map/set
lookups; it does not admit or meter comparison traversal work. Selected retained
backing remains governed by the existing domains. Remaining next slices are
nil/containment validation, detached temporary identity/argument copies, then
membership callable/recursion and declaration/binding setup work before combined
accounting and operation integration. No ABI, encoding, numerical or backend
policy change is intended.
### Nonallocating nil and containment validation (contribution awaiting review)

Following #280 review 5865274115, Uuid::nil compares its existing text against
the canonical nil literal instead of constructing a temporary Uuid. Containment
uses the same internal full-address validator as rendering and ObjectKey ordering,
then performs the existing scope/length/element-prefix checks. It no longer
renders keys solely to validate them. Both operands are validated before any
scope or length short circuit; empty, invalid-element and partial addresses keep
the same error message and rejection behavior. Rendering and ordering semantics,
UUID normalization, public representations and allocator ownership are unchanged.

The standalone test observes zero C++ new/new[] for valid nil and containment
calls. Nil controls cover canonical nil, a nonzero hexadecimal digit in every
UUID digit position (including uppercase input normalization), and an attached
nil whose domain has exactly exhausted its 37-byte budget. Containment is checked
against an independent rendered-text prefix oracle with explicit delimiter
boundaries, using multiple scopes, lengths 1/2/5/128, alphabet boundaries, strict
prefixes, equal keys and near misses. Invalid operands on both sides are checked
across different scopes and lengths. Separate negative controls link the new test
to the prior source and reject each old allocating implementation. Exception-path
allocations are outside the zero-allocation claim. Existing ordering, address and
UUID domain/lifetime tests remain active.

This removes these validation temporaries; it does not meter traversal work or
make all setup allocations selected. Next: detached temporary identity/argument
copies, then membership callable/recursion and declaration/binding setup budgets,
combined resources and operation/control integration. Preserve caller ownership,
detached snapshots and current rejection/retry semantics in those later slices.

### Adoption argument copies and temporary declaration identity (awaiting review)

Following #281 review 5865407113, ExchangeOwner and both ExchangeBindings
construction paths take occurrence/field vectors by const reference. They use
those references only during construction; the existing domain-owned ExchangeMap
copies still retain their own rows, UUIDs, paths and addresses. No caller table
is moved from or retained by reference. The legacy binding overload continues to
consume its declaration value intentionally; shared declaration/catalogue aliases
and detached snapshot behavior are unchanged.

The aggregate OccurrenceKey passed temporarily to LetterExchangeContract::collect
now constructs its UUID with the owner reservation domain. That character request
is admitted before allocation and released at the end of the same call expression;
its cumulative work remains charged. The declaration independently owns its
existing domain-bound copy. This accounts for the temporary without adding a
borrowed-identity API or changing lifetime/lookup semantics.

A source-death control adopts from local caller tables, checks their backing and
values are unchanged, destroys them, then verifies the retained owner mappings.
Existing byte/work refusal sweeps, exact retries, full allocation-site failures,
independent retained-byte/domain witnesses, detached snapshots and shared/weak
lifetime controls remain active. Input admission's logical byte/copy gate stays
conservative and unchanged; eliminating redundant argument copies does not remove
selected map copies or claim all constructor work is metered.

Next: audit remaining detached temporary identity copies outside these adoption
paths and address membership callable/recursion plus declaration/binding setup
budgets, then combined resources and operation/control integration. Domain object/
control-block overhead and caller/native ownership remain separately inventoried.

### Borrowed byte-expansion identities (awaiting review)

Review 5865626048 of #282 authorizes the remaining temporary-identity audit.
The private ByteRef rows returned by letter_projection.cpp's expand now borrow
character/byte ToolRefs from the source tool table. All three consumers finish
before that table is changed or destroyed: legacy and structural normalization
only append nodes/occurrences/selections, and project_letters reads a const bundle.
Published occurrence definitions and ByteProvenance still copy independent values.
The path vector and output tables remain owned; no public borrowing interface is
introduced. Both projection tests destroy their source bundle before validating
the resulting legacy/structural output, including UTF-8 provenance and revisions.

This is initial recipe/projection preparation outside owner adoption, not newly
selected owner allocation. A direct expansion probe observes 0 UUID-sized (37-byte)
character requests over 13 byte rows; the same probe linked to prior source
observes 52 and fails the zero-request check. It does not claim zero total
allocation: row/path backing and strings remain. Owner totals are not evidence
for this removal because adoption copies an already constructed projection.

Audit boundaries still outstanding: the initial projection's owning tool/occurrence
indexes and provenance/equivalence tables, temporary rendering keys and validation
strings; these must be separated from intentionally independent published outputs.
Owner projection copy helpers, contract copies and membership keys already construct
identities in their selected domains. Standalone coordinator map copies provide
candidate backing and cannot simply become borrowed aliases. Membership callable/
recursion and declaration/binding setup budgets remain open, followed by combined
resource accounting and remaining-operation integration.

### Occurrence rendering argument copies (awaiting review)

Following #283 review 5866277807, initial projection selection/part/contact text
rendering now passes the existing study and OccurrenceId to a private helper.
It no longer constructs an owning OccurrenceKey merely to render it. The public
occurrence_text delegates to the same helper and retains the nil-study and path
length predicate, error, normalization, decimal ranges and separator format.
The result string remains owned; no reference escapes. Retained projection and
snapshot copying, selected domain budgets and owner refusal/retry are unchanged.

A focused comparison of the old aggregate-key call shape and the component call
observes UUID-sized requests fall from 2 to 1 for both aggregate and byte paths;
byte-path requests fall from 1 to 0. The remaining UUID-sized request is output
rendering, not a temporary key. Full-range output and malformed-key controls are
in letter_projection_test. This neither eliminates output allocation nor completes
initial projection/index or owner setup accounting. Initial indexes/provenance/
equivalence copies, membership callable/recursion and declaration/binding setup
budgets remain open before combined resources and operation integration.


### Maintainer integration through #284

**Maintainer integration — 2026-11-11:** Reviewed #275–#277 and #279–#284 are integrated into main at `5beeb2cacf28c75cbecef385a0de1e877154bbc0`, preserving original commits through reviewed tip `09bafe48869ddf239e6e245d766c0c0e41a43364`. #279's fresh native 43115507493, viewport 43115507480 and smoke 43115507510 succeeded; #280–#284 completed their applicable hosted checks, including #284 native 43116769390 and smoke 43116769371. Original failed and stuck queued attempts remain historical, not relabeled. Earlier pending/integration labels below are superseded by this record. #285 is provisionally source-accepted at `7105c22e866af7c3cf5c1b2e452a8bf8bef643f8` (review 5866915489), targets main and awaits native qualification; its smoke passed. Next: initial tool/occurrence indexes and publication/setup costs, then membership callable/recursion, declaration/binding budgets, combined resource accounting and operation integration. No blanket GPU or performance acceptance.


### Temporary provenance/equivalence lookups (awaiting review)

Following #284 review 5866703067, initial projection's provenance lookup stores
only owner entry/ordinals and a borrowed character ToolRef. Its equivalence lookup
also borrows the pinned ToolRef. Both point into the input bundle's stable tool/
parent tables, not into short-lived expansion rows. project_letters takes that
bundle by const reference and never mutates it. When each published occurrence is
constructed, provenance/equivalence become independent owned identities again.
No pointer escapes. Lookup key ownership, ordering and the published record copy
remain unchanged; this is not a broad conversion of retained maps to borrowed data.

The source-death test now checks complete pinned character/equivalence references
against the original input after the projection's actual source copy is destroyed.
Repeated/multibyte characters, distinct letter entries and unknown equivalence
remain covered. A source-to-source initial-projection probe counts 840 versus 795
37-byte requests for five letters: 13 provenance rows lose three temporary UUID
copies each, and three equivalence entries lose two each (45 requests removed).
The observer is size-filtered, not a general allocation or performance proof.
Published output and owner-adoption accounting remain independent of that temporary
reduction. Initial tool/occurrence indexes, retained publication copies and their
setup costs require separate treatment; membership callable/recursion and declaration/
binding setup budgets remain open before combined resources/operations.

### Borrowed keys in the initial tool index (awaiting review)

Following #285 review 5866915489, the local normalization/projection Tools index
stores reference_wrapper<const ObjectKey> keys alongside its existing const Tool
pointers. Both refer into the same stable source tool table. Neither normalization
consumer changes tools/parents and project_letters takes its bundle const. The
index is destroyed before the source; moving the normalization result does not
make index destruction inspect the borrowed keys. No index or reference escapes.

ToolKeyLess delegates to the existing ObjectKey value comparator, including its
validation and ordering. Transparent find and at continue matching by value;
a separately copied lookup key resolves the same source Tool. Duplicate identity
and inconsistent revision rejection are unchanged. Normalization duplicate tests
cover both source orders; source-death projection tests still check independently
owned published values. A source-to-source index probe observes zero 37-byte UUID
requests over 22 tools, versus 22 in prior source. Map nodes still allocate;
this is not a zero-allocation or complete setup-budget claim.

Remaining initial occurrence indexes, publication copies/setup costs, membership
callable/recursion and declaration/binding budgets precede combined resources and
operation integration. Deliberately independent outputs must remain independent.


### Maintainer acceptance through #286 — 2026-11-12

**Maintainer acceptance — 2026-11-12:** #285 and #286 are integrated into main at `087b6aff14d454c9ebafcda28c9e9243f596e4d2`, reviewed tip `49643f2a22bbb0076584e01d990a32ff7fe0acda`. Exact-head #286 native 43130677310 and smoke 43130677322 passed; #285 native 43121173964 and smoke 43121173792 also passed. All 12 #286 native jobs succeeded. Personally inspected regression logs total 115 passing tests (29/29/29/28); database job 130894721312 reports addressed_reads=24/private_reads=0. The active review queue is clear; #75, #111 and #123 remain deliberately held. Earlier provisional/pending labels below are historical and superseded by this acceptance. No GPU or performance qualification is implied.

**Next bounded task:** start from current main and audit initial occurrence indexes in `project_letters` and their publication/setup costs. Preserve value ordering, duplicate/revision rejection, source stability, exact provenance and independent published outputs; distinguish removable temporary copies from necessary owned publication storage. Keep the implementation bounded with source-death and independent allocation evidence. Membership callable/recursion, declaration/binding setup budgets, combined resource accounting and remaining operation/control integration follow. Do not repeat accepted baseline tests merely to onboard.


### Ordered occurrence lookup values (awaiting review)

Starting from accepted main `35814207`, project_letters' ordered index keeps its
owned OccurrenceId key but no longer duplicates that ID inside a StructuralOccurrence
value. Its value now holds the node and a const ObjectKey pointer into the original
legacy/structural input row. That input remains const and stable throughout projection.
The value is private temporary metadata; published keys, definitions and text remain
independently owned. Ordering, duplicate/revision validation and the retained recipe
copy are unchanged. This does not borrow map keys or remove publication storage.

Both recipe-version source-death tests now verify every published occurrence's full
definition reference/revision and node against the original normalized input. A
focused initial-projection comparison against main observes 773→755 37-byte requests
and 223→210 8-byte requests for 18 occurrences, 13 with byte paths: one temporary
scope copy per row and one redundant path copy per byte row removed. Size-filtered
observations support the source change, not total allocation or performance claims.

Remaining owned occurrence-index keys, the legacy lookup, used-definition keys and
publication copies/setup require separate audit. Membership callable/recursion and
declaration/binding budgets follow before combined resource/operation integration.
