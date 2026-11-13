# Borrowed backing: native observability boundary

Status: source audit and bounded implementation handoff, awaiting review,
2026-11-12. Continues [#299 review 5879484896](https://github.com/Human-Cognome-Project/human-cognome-project/pull/299#pullrequestreview-5879484896)
at `5771326e4992889c3b9c16b8b98ee752ae4d0d1a`. Taichi source tree:
`5ba579a8a11255b7a258cde93801070956d81d57`. This is **not** an implemented B
ledger or combined admission gate. It identifies the precise missing native
request surfaces requested by that review, without replacing them with estimates.

## Program ownership scope follow-up (awaiting review, 2026-11-13)

The [Program accounting audit](PROGRAM-ACCOUNTING-SCOPE.md) follows #316 review5891228994
and traces existing ownership before further inspection changes. It distinguishes
retained Program host graphs from retired device backing, HCP-owned kernels from Program
kernel-vector entries, and compiler provenance/ABI metadata from implementation payloads.
The compiler exclusion and string-storage classification are concrete proposals for
review, not accepted policies. The next bounded exporter is Program object/vector storage,
with remaining map/deque/pointee coverage explicit; no parallel execution machinery.

## Existing thread-pool storage (awaiting review, 2026-11-13)

Continues [#315 review 5890749492](https://github.com/Human-Cognome-Project/human-cognome-project/pull/315#pullrequestreview-5890749492).
Source inspection found the executor already owns a native ThreadPool whose constructor
creates the configured workers and whose run method supplies synchronization and dispatch.
The existing ProgramImpl runtime_bookkeeping report now observes that owner directly.
ThreadPool construction, scheduling, worker state and compilation are unchanged; no
alternative thread machinery or native optimization is introduced.

The report separates pool object bytes from its std::thread vector capacity bytes.
Object size includes embedded synchronization representations and vector control storage;
the vector's element buffer is separate. Count these once per live pool owner, not per
worker, task, invocation or Harness. thread_handles counts constructed handle objects,
not currently executing workers, a concurrency budget or a performance observation.
The pool is separately allocated from the executor and root manager; their object sizes
do not contain its object or handle buffer. Capacity multiplication is checked.

Unsupported ProgramImpl defaults remain unavailable. A supported executor with no pool
reports available but absent, with zero identity/object/capacity. An existing pool is
present even if its vector is empty. Overflow marks the report unusable for aggregation;
zero bytes must not be treated as complete coverage. complete remains false in all states.
The query performs fixed metadata reads without allocation, enumeration, locks, waits,
worker-state inspection or OS queries. Keep the executor alive and quiescent; the returned
pointer is a synchronous identity, not a lifetime lease or teardown-safe token.

This exports object/capacity accounting, not all thread allocation requests. OS stacks,
kernel resources, implementation-private standard-library allocations, worker TLS and
other hidden resources are not measured. These omissions are disclosed, not newly accepted
exclusions from full B. Linux CPU fixtures cover one/two workers and retained storage
through existing array/tree work; absence during partial construction, thread-creation
failure and OS teardown reclamation are not exercised. GPU/native Windows remain unqualified.

Next: remaining Program ownership and concrete string/compiler/JIT scope decisions,
then trusted required-family/lifetime aggregation with canonical/domain deduplication
and prepaid work before C+B+H+D. The launcher nonempty-name/shape fixtures requested when
those fields are consumed remain deferred to that integration. No full B or combined
admission claim is made. See [coverage audit](RUNTIME-COVERAGE.md).


## Shared launcher registry capacity (awaiting review, 2026-11-13)

Continues [#314 review 5890456367](https://github.com/Human-Cognome-Project/human-cognome-project/pull/314#pullrequestreview-5890456367).
Common KernelLauncher inspection methods expose existing registry storage. ProgramImpl
forwards inspection only to an already-owned launcher: it never calls the lazy factory.
Absent means no launcher exists; an instantiated unsupported backend remains Unavailable.
The CPU override reports Present even for an empty registry, while invalid context
indices report Invalid. No compilation, registration, dispatch or allocation policy is
reimplemented; these methods observe the native owners already used for execution.

The outer report gives launcher object size, constructed context-slot count and outer
vector capacity bytes. Object size includes its vector control object; the element
buffer is separate. Constructed slots can include empty entries and must not be read
as proof of active kernels. One indexed context report supplies task-function and
parameter-vector capacity, plus constructed parameters' nested argument-path and
element-shape vector capacities. Parameter rows contain string/vector control objects;
nested element buffers are separate. Unconstructed spare slots are not traversed.

Parameter-name capacity+terminator is exported as a separate conservative charge,
not a disjoint allocation request: small-string storage may already lie inside the
parameter object. It must not silently be added to an actual-request ledger. An agreed
accounted-capacity policy or actual string-allocation classification is still required.
DataType pointees, executable code and compiler/JIT ownership are not followed or
claimed by this report. Both completeness flags remain false.

Outer inspection and context-index validation are constant-time. Before traversal,
the context checks its parameter count against the supplied budget. InsufficientWork
returns the required count with zero visited parameters. Exact-budget retry visits
each constructed parameter once, reading capacities without walking strings or path/
shape elements, copying data or allocating. Overflow yields an unusable partial report.
Checked products/additions cover capacity arithmetic. Outer enumeration, validation,
deduplication, registration/setup and other backing lookups remain separately payable.

The synchronous owner identity is the shared launcher, not an invocation. Count its
object/outer buffer once, then each selected constructed context's nested storage once.
Identify a context slot by the queried (launcher owner, index) while quiescent; the
report alone is not a durable context token. Invocation moves and repeat launches do
not create independent shared-registry charges.
Keep Program/launcher/runtime alive and quiescent; the identity is not a lifetime lease,
and retained capacity may change when registration occurs. GPU metadata inspection is
still deferred, not a detected compilation/device failure. HCP uses the common native
interface, with no new model-side backend casts.

Next: remaining Program/thread ownership and string/request-policy decisions, followed
by trusted Harness required-family/lifetime coverage, canonical/domain deduplication
and prepaid work before C+B+H+D. Compiler/JIT exclusion remains a proposal; no full B,
heap/physical-residency, GPU or native-Windows execution qualification is claimed.

## Runtime-global requests (awaiting review, 2026-11-13)

Continues [#313 review 5890147678](https://github.com/Human-Cognome-Project/human-cognome-project/pull/313#pullrequestreview-5890147678).
The existing native reservation interface now has a fixed runtime_global_reservations
catalog: LLVMRuntime object; root pointers/sizes; element-list, node-manager and ambient
pointer tables; existing ambient request-size metadata; temporaries; random states;
and the runtime result buffer. The ten-entry catalog contains AmbientMetadata exactly
once, retaining its existing enum value and request. Other existing enum values remain
unchanged. Supported ReservationLimits exposes the catalog count; unsupported defaults
remain unavailable with zero count.

Every global selector requires node=-1, Elements, index=0. Native queries read the
actual stored initialization capacities/counts and pointers, with checked multiplication
and invalid negative-count refusal. Object/result alignments preserve CPU requests128/8;
the tables/temporary/random requests preserve4096. LLVMRuntime object size is computed
inside its owning runtime module, not reconstructed by HCP. No allocation behavior,
owner fields, native request size or buffer layout changes.

The existing CPU query bridge remains allocation-free and synchronous under a live,
quiescent owner. GPU query coverage remains unavailable: no host dereference of GPU
runtime state and no inferred preallocation coverage. GPU result storage is contained
in preallocated runtime-object backing and still requires its own supported export/
qualification; this change does not certify it.

reservation_pool_backing resolves each Present CPU request through the existing
canonical host-pool interface. Distinct global requests can share one pool token:
containing raw/rounded backing and contained requests are not disjoint byte charges.
The request report itself still does not claim canonical/rounded coverage; the separate
bridge supplies that evidence. Global result storage is distinct from prepared
invocation result buffers. Tree destruction does not retire these runtime-global
reservations; runtime lifetime, not tree generation, governs their views.

Catalog length permits explicit bounded enumeration, not free work. Each request
query retains its fixed native lookup unit; the pool bridge repeats that query and
performs the existing logarithmic pool lookup. Caller/Harness enumeration, validation
and deduplication must be prepaid separately. This is retained request observability,
not a complete ledger, setup peak or physical-residency measurement.

Next: shared launch-registry metadata and remaining Program/thread ownership or
reviewed scope decisions, then trusted Harness required-family/lifetime coverage and
canonical/domain deduplication before combined C+B+H+D. Compiler/JIT exclusion remains
proposed, not accepted; the [coverage audit](RUNTIME-COVERAGE.md) retains that distinction.

## Scoped runtime coverage audit (awaiting review, 2026-11-13)

The [source-traced coverage and policy matrix](RUNTIME-COVERAGE.md) follows #312
review5890043642. It distinguishes current partial exports from required-family
blockers and proposed exclusions, including the unexported runtime-global tables/
buffers and shared launch registry. A compiler/JIT exclusion is proposed for review,
not accepted or implemented. Existing selected-D exclusions do not silently remove
borrowed B obligations. Top-level availability never replaces per-family coverage.
Next code slice: runtime-global object/table/temporary/random/result requests through
native/common interfaces. This audit performs no runtime qualification or gate change.

## Runtime bookkeeping tables (awaiting review, 2026-11-13)

Continues [#311 integration handoff](https://github.com/Human-Cognome-Project/human-cognome-project/pull/311#issuecomment-6850662795)
on main c5645a28. ProgramImpl::runtime_bookkeeping() now provides a common inspection
surface for LLVM executor/root-manager objects and three retained allocation tables:
root records, tree allocation descriptors and runtime allocation descriptors. Other
implementations default to unavailable; an empty supported map still reports its
domain/counter storage. A missing root manager has no identity/object charge and an
unavailable root-record report. complete remains false.

The tables use the reviewed shared request-counting allocator. Each report separates
actual live node/bucket requests, counter-object request and actual counter control-block
request, with synchronous domain identity and entry count. Map value payload is a
subset of allocation requests. Existing incremental root-record bytes are a subset of
root-map values, never an extra charge. These maps have fixed-size keys/values, with no
nested key buffers. The root-manager object is separate from the executor base object;
each includes its embedded container/allocator controls through sizeof. Executor base
size does not assert coverage of an arbitrary derived executor or of ProgramImpl.

These are retained bookkeeping allocations, not a live native backing registry.
Ndarray destruction can retire Device backing while leaving an executor descriptor.
Tree destruction removes the root-manager entry and its owned allocation but retains
the executor's tree descriptor. Such entries still occupy host map storage and must
remain counted; they must not be interpreted as evidence of live payload backing.
Original cleanup, identity and allocation behavior are unchanged apart from allocator
bookkeeping and its additional setup allocations.

Inspection performs a fixed number of size/counter reads, without enumeration,
allocation, native pointer following or backing lookup. Payload multiplication is
checked; unavailable/overflow map reports cannot establish required-family coverage.
Keep runtime, manager and all allocator domains alive and quiescent, and count each
object/domain once across consuming Harnesses. Raw identities do not extend lifetime.
Finalization's map copy selects a fresh domain; this snapshot is retained storage,
not temporary copy/rehash peak accounting or setup-work prepayment.

Still excluded: Program/ProgramImpl and compiler/JIT ownership trees, thread-pool and
profiler internals, other runtime containers/buffers, backend/driver/process resources,
host-heap internals and physical residency. This is not full B or combined admission.
Next: remaining shared runtime surfaces and an explicit scoped coverage/exclusion
policy, then trusted Harness lifetime/required-family coverage and canonical/domain
deduplication with prepaid enumeration/lookup before checked C+B+H+D.

## Prepared map allocation requests (awaiting review, 2026-11-12)

Continues [#310 review 5883920961](https://github.com/Human-Cognome-Project/human-cognome-project/pull/310#pullrequestreview-5883920961).
The four native context binding maps now use SharedRequestCountingAllocator. Rebound
allocations record checked actual n*sizeof(allocated type), including standard-library
nodes and bucket arrays; matching deallocation removes those requests. The budgeted
metadata query reports that retained request total separately from key-vector capacity.
Value payload is a subset of node requests and must not be added again. Bucket count
remains diagnostic; it is not multiplied by an assumed pointer or bucket size.

Each allocator domain owns one separately allocated SharedRequestCounter plus one
shared_ptr control block. Both requests are exported: sizeof(SharedRequestCounter)
for the separate counter object and the actual rebound control-block allocator request.
The control-block observer writes only after successful allocation; its deallocate
does not dereference the observation pointer because shared_ptr deletes the counter
before freeing its control block. Constructor failure relies on shared_ptr's deleter
cleanup. This is requested storage, not malloc rounding or physical residency.

Allocator rebind/copies share stable counter lifetime. Move assignment propagates that
domain after releasing displaced storage; moved-from allocators retain a valid shared
domain. Ordinary container copy construction selects a fresh domain, so an independent
map copy does not contaminate the source's count. A domain can still be shared through
moves or explicit allocator copies. Reported allocation_domain is a synchronous identity:
count node/bucket requests and counter/control-block storage once per domain, not once
per aliasing map/context. All related containers must remain alive and quiescent during
inspection. Reuse after destruction is not certified; the raw identity is not a lease.

The context object includes the enlarged allocator/control members through sizeof.
Counter object, control block, node/bucket requests and key element buffers are separate
requested-storage surfaces; vector control objects/value payload are included in nodes.
The maps are publicly accessible native containers, so explicit allocator-sharing users
must preserve domain-wide synchronization/deduplication. This is not a new thread-safe
container or a replacement for the future trusted Harness ownership boundary.

The existing entry preflight and key enumeration budget remain unchanged. Counter reads
add constant work per map, do not allocate, and do not prepay setup/rehash/peak work.
Additional setup allocations are intentional. No numerical/launch policy changes or
model-side backend casts are introduced. complete and host_metadata_complete remain
false: this export is not all prepared/runtime/shared metadata or physical memory.
Next: remaining runtime/shared surfaces, then lifetime-valid required-family Harness
coverage and canonical/domain deduplication with prepaid work before checked C+B+H+D.

## Prepared host metadata (awaiting review, 2026-11-12)

Continues [#309 review 5883627408](https://github.com/Human-Cognome-Project/human-cognome-project/pull/309#pullrequestreview-5883627408).
PreparedBacking now reports scalars.capacity()*sizeof(Prepared::Scalar), including
unused slots. The vector control object is already included in prepared_object_bytes;
the capacity buffer is separate. This does not change prepared_bytes(), reserve policy
or execution behavior.

The separate prepared_host_metadata(live_device, entry_budget) inspection covers the
four context binding maps: runtime sizes, allocation types, array pointers and argpack
pointers. It reports each map's entry count, bucket count, value payload and the sum of
owned key-vector capacities in bytes. Key element buffers are separate from map value
payload, whose vector control object is embedded in each value. Different map keys
own distinct buffers even when they contain equal argument paths.

Four constant-time size observations and checked addition establish required_entries
before any traversal. A smaller budget returns unavailable with zero visited_entries;
an exact-budget retry visits each entry once without copying, hashing keys, following
bound pointers or allocating. Each visit reads capacity in constant time; it never
walks the key elements. Multiplication and accumulated key-byte arithmetic are checked.
Overflow returns unavailable/overflow; partial fields must not be accepted as a ledger.
The entry budget covers this inspection only, not prior setup, allocation, native
backing lookups, synchronization or later Harness work.

Bucket count is diagnostic, not bucket allocation bytes. Map value payload is a subset
of still-unmeasured node allocation storage, not a complete node-size estimate or an
additional charge once actual map requests become available. Empty maps may retain
buckets. complete and host_metadata_complete remain false: node/bucket storage,
shared runtime/compiler metadata and host-heap internals remain outside this export.
No physical-memory or full B claim is made.

The same prepared-buffer availability guard rejects unsupported, unprepared, unusable,
foreign-device and moved-from contexts. Caller must keep context/kernel/runtime/device
alive and quiescent; returned values are synchronous observations, not durable identity
or a lifetime lease. A future trusted Harness must deduplicate contexts and native
backing under its exclusive boundary and prepay all other required enumeration/lookup.
No new model-side backend casts or retained metadata fields are introduced.

## Pool container metadata (awaiting review, 2026-11-12)

Continues [#308 review 5883275400](https://github.com/Human-Cognome-Project/human-cognome-project/pull/308#pullrequestreview-5883275400).
PoolBackingStorage now separately reports live raw-map allocation request bytes,
UnifiedAllocator object bytes, and retained chunk-vector capacity bytes, with explicit
record/allocator storage availability. The existing record_payload_bytes and cumulative
incremental payload remain subsets of map allocation bytes, not extra disjoint charges.
The allocator object includes its vector control object; the vector element buffer is
separate. Pool object size includes its embedded map, counter and allocator state.

The raw map uses a stateful rebinding RequestCountingAllocator. Each successful
allocate(n) adds checked n*sizeof(actual allocated type) to the owner counter; failure
leaves it unchanged and deallocate subtracts the matching request. This measures the
standard library's actual node allocation requests rather than guessing node headers
from value sizes. It does not measure malloc usable size, heap rounding or physical
residency. Empty-map implementation storage, if allocated, is included too.

The counter is embedded before the map so it outlives the map and all rebound allocator
copies. Pool mutation/query synchronization protects it; it is not a separately allocated
registry. Internal reset map copies share that counter while they exist, and destruction
removes their requests. The locked snapshot reports retained state after reset, not a
transient peak or a prepayment mechanism for reset's temporary copy.

Chunk-vector storage is capacity*sizeof(MemoryChunk), including unused retained slots.
Erasing an exclusive chunk can remove map storage while retaining vector capacity;
reset replaces the allocator and releases that buffer. The fresh allocator object is
still retained. Availability distinguishes known empty storage from unsupported defaults.
A live-allocation witness compares retained map/vector growth with C++ new requests
still live after allocation. Cumulative requests also include temporary storage and are
reported separately. Tests neither hard-code node size nor presume empty-map overhead is zero.

Count these disjoint requested-storage surfaces once per pool identity: pool object,
map allocation requests, allocator object and chunk-vector buffer. Do not additionally
add map value/incremental payload. These are separate from mapped data backing, whose
canonical tokens/rounded virtual extents are unchanged. Full metadata_complete remains
false: process-global state and underlying host-heap bookkeeping/rounding are outside
this report, and complete runtime/prepared/Harness coverage remains outstanding.

Queries read counters/capacity under the existing pool lock, without enumeration or
allocation. Allocation-side arithmetic rejects multiplication/addition overflow before
publication. Native allocation failure cleanup and owner/backing lifetime rules remain
unchanged. Next: remaining prepared/runtime/shared metadata and trusted Harness coverage,
lifetime/deduplication with prepaid lookup/enumeration before checked C+B+H+D admission.
No full B, physical memory, Windows-execution or GPU qualification is claimed.
## Rounded host-pool extents (awaiting review, 2026-11-12)

Continues [#307 review 5882509096](https://github.com/Human-Cognome-Project/human-cognome-project/pull/307#pullrequestreview-5882509096).
Native host-pool setup captures the OS page size using sysconf(_SC_PAGESIZE) on Unix
or GetSystemInfo on Windows. Unknown/zero page size fails setup rather than inventing
an extent. Before each raw mapping, checked remainder/padding arithmetic rounds its
nonzero request to that page size, rejecting overflow before an OS call or accounting
publication. The allocator passes the rounded length to mmap/VirtualAlloc and stores
it in the successful allocation record. Cleanup and Unix unmapping use that retained
length; Windows releases the whole mapping with MEM_RELEASE as before.

This follows the ordinary-page mapping contracts in the
[Linux mmap reference](https://man7.org/linux/man-pages/man2/mmap.2.html) and
[Windows VirtualAlloc reference](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc).
The value is the successfully allocated virtual mapping extent for this native path,
not physical residency, a huge-page measurement or a promise of available RAM.
The Windows branch is source-level implementation; this contribution's execution
qualification remains Linux/WSL CPU only, with native Windows/GPU runs outstanding.

Live PoolBackingRecord queries now set rounded_extent_available and return the retained
rounded_extent_bytes. Raw request bytes remain separate and unchanged. Containment,
UnifiedAllocator chunk tails, Device allocation records and prepared registration
bounds still use original request sizes; OS padding never enlarges a valid request or
view. ReservationRequest still does not report an independent rounded extent for a
pool suballocation; callers use the separate underlying pool record. Unknown,
Unavailable and Absent records do not fabricate usable rounded coverage.

Pool statistics.reserved_bytes retains its existing sum of raw request sizes; it is
not silently redefined as rounded bytes. Future backing accounting must use one live
canonical pool token/extent per shared raw allocation, never add its suballocation
requests again. No Harness aggregation/deduplication or physical-memory total is built.

The retained rounded length adds one size_t per raw map value; the captured page size
adds one pool field. Existing whole-object and sizeof-based map payload reports include
both once per pool. Incremental record payload remains the cumulative delta from the
original size-only map, including identity and rounded-length fields, a subset rather
than an additional charge. Full pool/allocator/map-node/global metadata remains
incomplete. Queries read stored lengths without OS calls or allocations; setup and
allocation work is outside the lookup path, whose logarithmic cost still needs metering.

Next: remaining prepared/runtime/pool metadata, then lifetime-valid required-family
Harness coverage and canonical backing deduplication with prepaid lookup/enumeration
before checked C+B+H+D admission. No complete B or GPU qualification is claimed.
## Native host-pool backing identity (awaiting review, 2026-11-12)

Continues [#306 review 5881942652](https://github.com/Human-Cognome-Project/human-cognome-project/pull/306#pullrequestreview-5881942652).
The native HostMemoryPool raw-allocation map now retains allocation identity alongside
its existing exact raw request size. A process-wide nonwrapping identity generator
assigns distinct pool instances and raw allocation identities. Tokens contain pool
instance, allocation identity and base address; an address or movable allocator chunk
index alone is never identity. Release removes the record; reset does not reset the
identity generator. Exhausted identity leaves coverage Unavailable rather than reusing
an identity. Current token lookup returns Unknown after release/reset/replacement.

`capture_backing_record(address, bytes)` establishes only containment in a currently
tracked raw allocation. It requires an independently established live nonzero request;
it is not proof that an arbitrary address names a valid suballocation. Zero-size/null,
cross-boundary and overflowing-size ranges return Unknown. Lookup uses an ordered
integer-address map and subtraction bounds, never a full chunk walk or pointer
arithmetic across allocations. Captured `backing_record(token)` validates identity
without dereferencing saved addresses. Both lock the native pool and allocate nothing;
they do not extend request/owner/pool lifetime or prevent a later reset.

Common `Device::allocation_pool_backing_record` first validates canonical Owned
provenance, then maps that live owner's range to raw backing. Explicit owned imports
resolve to the same pool token as their owner; raw imports remain Unknown. A live
zero-byte CPU owner has no raw mapping and reports Absent. Common
`ProgramImpl::reservation_pool_backing` first obtains the native CPU request and then
resolves its range; absent requests stay Absent, invalid selectors yield Unknown, and
unsupported implementations stay Unavailable. The original request query retains its
own status and flags. These separate bridges do not add model-side backend casts.

The result reports `raw_requested_bytes`, not OS rounding, committed/resident physical
bytes or a per-Harness total. `rounded_extent_available` remains false and rounded
bytes remain unusable zero. Distinct suballocations sharing a raw chunk share its token:
never add the chunk and its contained requests as disjoint backing. This resolves raw
host-pool identity for supported live requests only; it does not establish exhaustive
owner-to-pool membership or account unused/orphaned pool reservations for a Harness.

`pool_backing_storage` reports the pool instance, whole HostMemoryPool object size and
live map value payload bytes. Incremental payload is the new value layout minus the
prior layout and is a subset of total payload. The added pool-instance field is already
inside the object size. Count this storage once per shared pool, not per Device/tree.
Map node/allocator overhead, UnifiedAllocator object/vector capacity, and the shared
process-wide atomic identity generator remain excluded; metadata_complete is false.
CPU device construction materializes the shared pool bookkeeping before inspection;
the first device metadata query does not perform lazy setup or allocate a raw mapping.
Raw-map registration failure now releases its acquired OS mapping before propagating
failure. Allocation identity may be consumed on failure but is never reused.

Pool map lookup is logarithmic in tracked raw allocations, with synchronization; it is
not the reservation query's fixed-depth one-unit lookup. Metadata size queries are
constant-sized. The eventual Harness must explicitly meter these lookups and any
input-sized enumeration/validation before combined admission; no complete work gate
or hidden snapshot is introduced here. The caller keeps device/runtime/request owners
live and quiescent and the shared pool intact; resetting the pool while owners remain
live is outside the contract. Tokens are not lifetime leases or execution guards.

Next: authoritative rounded extents, remaining prepared/runtime/pool metadata, then
trusted lifetime-valid required-family coverage and canonical backing deduplication
with prepaid work before checked C+B+H+D. No full B, other-backend or GPU qualification.
## Prepared owned-import adoption (awaiting review, 2026-11-12)

Continues [#305 review 5881732238](https://github.com/Human-Cognome-Project/human-cognome-project/pull/305#pullrequestreview-5881732238).
Prepared contexts now expose `prepared_array_backing_record(live_device, argument)`
separately from `prepared_array_record`. The latter still reports the captured
registration, including Imported identity/view extent or Retired status. The new
query validates that registration and its bound byte extent before asking the common
Device interface for canonical live Owned backing. It never substitutes a new numeric
handle, infers ownership from an address, or casts to a backend in model code.

The bound element count must fit within the registration's extent using division
before canonical resolution. This prevents a view larger than its imported range
from becoming covered merely because the original owner has enough total bytes.
Type/rank/shape validation at prepare remains unchanged; this additional check is
accounting coverage, not an automatic launch guard. The oversized-view negative test
is prepared but never launched. Unknown queries supply no usable backing bytes.

`prepared_backing().array_bindings_complete` now requires canonical live Owned backing
for every array binding, accepting explicit same-device owned imports whose owners
remain live. Raw imports, foreign/stale/retired/unavailable bindings, external arrays
and unusable contexts still cannot complete coverage. A live import registration with
a retired owner remains Live through the registration query but Unknown through the
canonical query. Import retirement invalidates only that binding; the owner remains
usable. Context moves preserve captures/owner resolution and disable moved-from views.

No new retained fields or registry are added. Existing ArrayBinding/Prepared object
and capacity accounting is unchanged. One canonical binding query performs at most
three indexed record validations on CPU. The summary already scans array_bindings;
it remains input-sized work and must be prepaid/metered by the future Harness,
including this added validation. Fixed per-binding depth does not make traversal free.

Known-live quiescent context, Kernel, Program, Device, registration and backing owners
remain required for execution. Queries grant no lifetime lease or launch-time guard.
Full owner records enable future deduplication; this API does not itself aggregate or
deduplicate a Harness ledger. Host metadata remains incomplete, and pool backing
identity/rounded extents, remaining metadata and lifetime-valid work-metered combined
C+B+H+D admission remain outstanding. No full B or GPU qualification is claimed.
This section supersedes the earlier statement that all prepared Imported registrations
necessarily leave coverage incomplete, only for the explicit proven-owned path.
## Explicit owned imports (awaiting review, 2026-11-12)

Continues [#304 review 5881459672](https://github.com/Human-Cognome-Project/human-cognome-project/pull/304#pullrequestreview-5881459672).
Common Device now offers `import_owned_memory(owner_token, offset, size, out)` and
`allocation_backing_record(registration_token)`, implemented for CPU. The new import
path requires a currently live Owned token from the same device and an in-bounds
range, then derives its pointer from that owner. It captures provenance at creation;
it never scans registrations or infers ownership from a later pointer match.
Raw `import_memory(ptr,size)` remains unchanged and unresolved, even if its address
matches a current owned allocation. Existing raw imports are not retroactively
upgraded. Cross-device imports and nested imported-owner tokens are unsupported by
this bounded path; use the original owned token for multiple views.

The import keeps its own Imported registration token and view extent, with zero
owned request bytes. The separate backing query returns the original owner's live
Owned token and full request/extent, allowing a future ledger to deduplicate by
actual captured owner identity. Direct owned queries return themselves. A retired
registration, retired owner, foreign/stale/invalid token or unresolved raw import
returns Unknown without usable sizes; unsupported backends remain Unavailable.
A zero-size view, including at the end of a nonzero allocation, still resolves to
its actual owner; zero-size owned allocations retain their own identities.

Known-live quiescent device/backing owners are required. This is not a lifetime lease
or an automatic guard on existing map/transfer/launch APIs. An import does not keep
its owner alive. Releasing an import never frees the backing; releasing the owner
invalidates backing resolution while the import's registration may remain Live.
The caller must not execute through such an import. Reset epochs and device-instance
identities prevent old relationships reviving after allocation/device reuse. Pool
reset while device resources are live remains outside the supported lifetime contract.

The captured token is embedded in each existing CPU AllocInfo row, including owned,
raw-import and retired rows. `allocation_record_storage()` already measures retained
capacity times sizeof(AllocInfo), so it includes the added metadata once per device.
Do not add the token payload a second time. Registration can grow that existing vector;
its exception leaves the owner and output unchanged, and a retry can succeed. No
separate registry is added. Query uses at most two indexed record validations and
allocates nothing; it does not enumerate or establish a complete Harness snapshot.
The caller must still meter input-sized enumeration and validation.

This completes only explicit same-device owned-import provenance. Prepared contexts
continue to treat Imported registrations as incomplete until their separate binding
coverage path deliberately adopts and validates canonical backing. Underlying shared
pool backing/rounded extents, general raw-import ownership, remaining prepared/runtime
metadata, trusted Harness lifetime/coverage and combined C+B+H+D remain outstanding.
No full B, other-backend support or GPU qualification is claimed.
## Native reservation request exports (awaiting review, 2026-11-12)

Continues [#303 review 5880964608](https://github.com/Human-Cognome-Project/human-cognome-project/pull/303#pullrequestreview-5880964608).
Common ProgramImpl now exposes `reservation_limits()` and one indexed
`reservation_request(kind, node, list, index)`. The LLVM CPU implementation queries
its native runtime module, not a model-side backend cast or a logical capacity estimate.
This supersedes the earlier unavailable node/list/ambient rows for current reachable
CPU **request views only**. It is not a complete B ledger or combined admission gate.

The query distinguishes Present, Absent, Invalid and Unavailable. Supported empty
slots are Absent; out-of-range node/page/chunk/role selectors are Invalid; unsupported
backends or unavailable owner state remain Unavailable. Only Present returns an
address, exact allocator-request bytes and requested alignment. Native request views
are distinct from Device allocation records: these reservations are pool suballocations,
not independently registered Device allocations.

| Request kind | Actual owner/request source |
|---|---|
| NodeManager | Runtime create request for the native manager object |
| Ambient | Exact size passed to runtime_allocate_ambient, captured on successful allocation |
| ListObject | Element/free/recycled/data ListManager create request |
| ListPage | Only an allocated directory page; exact chunks_per_page × pointer-size request |
| ListChunk | Only an allocated chunk; immutable element_size × elements_per_chunk used by its allocation call |
| AmbientMetadata | The added size table once per runtime, selected with node=-1 and default role/index |

No request is synthesized for untouched sparse capacity. List object sizes include
embedded directory roots; separately allocated pages and chunks are separate requests.
Node payload slots within a chunk are not additional backing requests. Equal-size
list objects retain distinct addresses while the owner is alive. Ambient sizes are
stored because they cannot safely be inferred from logical occupancy or an unrelated
root record. Direct mutation/reinitialization of native owner internals is outside
this query contract; the view covers currently reachable requests, not an allocation
history registry.

**Coverage and lifetime:** requested bytes are known, but rounded extents and underlying
pool backing identity remain explicitly unavailable (`rounded_extent_available=false`,
`backing_identity_complete=false`). Never add the shared pool reservation and these
suballocations as disjoint resources. HostMemoryPool's alignment gaps/raw chunks and
imported-to-owner alias resolution remain separate work. The runtime-instance value
associates a view with its live CPU owner; neither it nor an address is a durable backing
token or lifetime lease. Query only through a known-live quiescent Program/runtime with
its pool intact, never through a dead owner or after externally resetting its pool.
Tree destruction frees its root but retains node/list/ambient requests in this runtime;
queries correctly keep those retained requests visible. They disappear with runtime/pool
teardown, not merely with a logical tree removal.

**Work and metadata:** every query performs one fixed-depth lookup; it never enumerates
all nodes/pages/chunks. Limits expose configured node capacity and shared directory
bounds with one logical query-work unit. The future trusted Harness must prepay and
meter its input-sized traversal/validation, restrict enumeration to owned resources,
and reject insufficient coverage before combined enforcement. This API itself does not
implement that admission gate. The CPU callback is resolved once at runtime materialization
and cleared on finalization, so inspection performs no JIT symbol lookup or heap allocation.

The new ambient-size table requests sizeof(uint64) × node_capacity bytes once per
runtime, including unused slots; AmbientMetadata exports its address/request. Native
preallocation requirements include the table on all LLVM architectures. The table pointer
and cached CPU callback pointer are reported as incremental embedded owner bytes, a subset
of runtime/executor object storage, not an extra charge when those whole objects are counted.
No per-node request registry or per-query storage is allocated. Shared JIT/compiler state
and the full existing runtime metadata census are not claimed by this incremental report.

Next: resolve underlying pool backing and imported aliases, finish remaining prepared/
runtime metadata, then assemble the lifetime-valid, work-metered Harness snapshot and
checked disjoint C+B+H+D admission. Other backend exports and GPU execution remain unqualified.

## Prepared CPU buffer exports (awaiting review, 2026-11-12)

Continues [#302 review 5880581671](https://github.com/Human-Cognome-Project/human-cognome-project/pull/302#pullrequestreview-5880581671).
This slice adds common `LaunchContextBuilder::prepared_backing(live_device)` and
`prepared_array_record(live_device, argument)` queries, implemented for CPU prepared
invocations. Unsupported/unprepared/unusable states do not report available coverage.
It supersedes the prepared-invocation row of the earlier owner-export table below
for these CPU surfaces only; no complete B ledger or combined admission is built.

The CPU owner exports actual argument/result allocation pointers and their validated
request sizes, its owned RuntimeContext request, and separate context/prepared object
sizes. A zero-sized result allocation is an available zero request, not unavailable
coverage. CPU preparation owns no separate device launch buffers, so that family is
explicitly complete and empty. Other launchers leave this coverage unavailable;
this change does not infer CUDA ownership from byte estimates or qualify GPU execution.

Array identities are captured from Ndarray at binding time into existing ArrayBinding
rows. Queries use those tokens through the independently known-live supplied Device;
they never recapture a potentially reused numeric ID or follow a saved device pointer.
Partial views retain full allocation extent and same-allocation aliases retain equal
tokens. Scalar and out-of-range arguments, foreign devices, retired allocations and
unavailable records remain distinct. Imported records remain unresolved registrations:
even a live import prevents complete array coverage until the real backing owner is
resolved. External/raw bindings are unavailable rather than silently counted as zero.
Rebinding through external/raw APIs clears the prior captured record.

These are synchronous, non-owning views. Keep the context, its Kernel/Program/runtime
and supplied Device alive and quiescent while inspecting. Returned host pointers do
not extend lifetime or certify identity across destruction/address reuse. Context move
transfers the buffers and records; the moved-from context reports unavailable. Frozen
contexts may change scalars through the supported setters; direct edits to public
native internals are outside this owner contract. The future trusted Harness snapshot
must enforce lifetime and reject caller-authored evidence; these records are not
security capabilities or a serialized authority.

Required-family coverage remains separate:

| Family | Coverage in this slice |
|---|---|
| CPU argument/result/owned RuntimeContext buffers | Actual pointers and validated request extents under live-owner precondition |
| CPU separate device launch buffers | Complete empty family |
| Bound arrays | Complete only when every captured array resolves Live and Owned; aliases still need deduplication |
| Full prepared host metadata | Explicitly incomplete; nested maps, scalar backing and other metadata are not a complete census here |
| Other backend launch buffers | Unavailable |
| Node/list/ambient reservations | Still unavailable through the owner-export interface |

The added AllocationRecord is embedded in each existing ArrayBinding row. Reported
`array_binding_capacity_bytes` includes it, and existing prepared_bytes/reserve math
already uses sizeof(ArrayBinding). `incremental_record_capacity_bytes` is a subset
for review, never an extra charge to add to that full capacity. No additional record
vector or allocation site is introduced. The new Prepared fields belong to its object
size, charged once with that object; neither object sizes nor buffer extents may be
added again through overlapping owner totals. The full metadata flag stays false.
Runtime-owned metadata remains a once-per-runtime charge, not a per-Harness/tree fee.

Next: node/list/ambient request exports, remaining prepared host/backend metadata,
imported-to-owner alias resolution and a trusted lifetime-valid Harness snapshot,
then checked disjoint C+B+H+D admission with the separate work gates retained.

## Array and root owner exports (awaiting review, 2026-11-12)

Continues [#301 review 5880096613](https://github.com/Human-Cognome-Project/human-cognome-project/pull/301#pullrequestreview-5880096613).
This first owner-export slice exposes actual array and tree-root records; it does
not assemble a Harness ledger or enable combined admission.

`Ndarray::backing_record(live_device)` retains the record identity captured when
the array is constructed. The caller must independently keep the supplied Device
alive and inspect under exclusive/quiescent access. Borrowed constructors require
a current handle at construction. Queries use the captured token, so release is
Retired and reset/reuse is Unknown rather than recapturing a new allocation at the
same ID. A supplied foreign device is Unknown without dereferencing the saved
handle's device pointer. Array shape does not determine backing extent: partial
views return the same full allocation record. Imports remain Imported registrations;
the export does not infer an underlying owner from pointer equality or count an
import extent as disjoint backing. Mutating public legacy handles is not a supported
way to rebind a captured owner record; construct a new view instead.

`ProgramImpl::snode_root_backing_record(tree_id)` delegates to the LLVM native tree
buffer manager. It captures the device record when registering the root, and queries
that token through the live runtime. Missing/destroyed roots are Unknown; unsupported
program implementations default to Unavailable. Destroying a root removes its export,
while an independently retained value token reports Retired on the still-live CPU
device. Equal-size roots remain distinct. Duplicate registration refuses replacement;
failed map registration releases the acquired device allocation before rethrowing.
The exported request is the actual request passed to the device after native root
page rounding, not the logical field payload and not OS residency.

Required-family coverage remains explicit:

| Family | Current export coverage | Complete ledger consequence |
|---|---|---|
| Ndarray backing | Captured common allocation record; CPU live owned records observable | Imported/Unknown/Retired/Unavailable records cannot certify disjoint live backing |
| SNode root | Native LLVM owner export; CPU record implementation | Root alone does not cover sparse reservations |
| Node/list/ambient reservations | Unavailable through these exports | Complete ledger must remain unavailable |
| Prepared invocation backing and bindings | Not exported by this slice | Complete ledger must remain unavailable |
| Harness host objects/capacities and nested owner enumeration | Pending owning snapshot policy | Complete ledger must remain unavailable |

Metadata policy: each Ndarray embeds one AllocationRecord, included once in its
object size, never added again when an enclosing object census includes it. The
existing root map embeds one record per live entry. The optional
`ProgramImpl::snode_root_record_storage_bytes()` reports this incremental record
payload once per Program/runtime, not once per tree or Harness. Zero means a supported
empty record map; nullopt means unavailable. It excludes pre-existing map nodes,
allocator overhead and root payload. Erased root entries retain no record payload.
This incremental report is not a complete native metadata census; the future ledger
must reconcile the existing map/device storage and shared-runtime ownership without
double counting. Queries add no per-call heap or registry storage.

Next: prepared invocation owners and node/list reservations, imported-to-owner alias
resolution, then the trusted lifetime-valid Harness snapshot and checked C+B+H+D.
No HCP model backend casts, new execution authority or GPU qualification are added.

## CPU allocation records (awaiting review, 2026-11-12)

#300 review 5879635043 accepts the CPU-first native interface slice. The current
contribution adds `capture_allocation_record`, `allocation_record` and
`allocation_record_storage` to common native `Device`, with CPU implementation.
The source audit below remains pinned to #299; this section supersedes its missing
common-query statement for CPU only. The complete B ledger is still not built.

The record separates `requested_bytes` from `extent_bytes`, and reports Live,
Retired, Unknown or Unavailable plus Owned/Imported/Unknown ownership. CPU exclusive
allocations use equal original-request and backend-request extents; neither means
OS page residency. Other backends inherit Unavailable and unavailable metadata
coverage, not an accepted zero-byte record. GPU rounding fields are not fabricated
from a logical array shape. Implementing those backends remains separate work.

Imported CPU registrations are marked Imported: requested_bytes is zero because
this device made no backing request, while extent_bytes is the supplied external
extent. This is registration metadata, not proof of the external allocation's
ownership, identity or lifetime. Releasing/clearing a registration does not free
external memory. Do not add its extent to owned backing until the real owner and
aliases are resolved. Zero-sized owned allocations have an explicit live bit;
null pointer alone no longer determines whether a record has retired.

Tokens are values containing a process-local device instance, reset epoch and
allocation ID. The common device instance counter never wraps into reused IDs;
identity exhaustion disables CPU record coverage. CPU allocation IDs append within
an epoch, and `clear()` releases live owned requests, retires imports without
freeing external memory, clears rows and changes the epoch. Epoch exhaustion also
disables coverage. Record-vector capacity remains retained and observable after
clear. Unknown tokens return no extent; retained retired rows preserve their
original extents until clear. Native stream/publication generation is unrelated.

**Lifetime precondition:** token inspection requires an independently known-live
Device under exclusive synchronous access. Tokens contain no saved Device pointer,
do not extend device lifetime, and are not serializable authority or capability
credentials. After the owner dies, do not query through its old pointer to discover
expiry. A new live device rejects the previous instance token even if its object
address is reused. Capturing a token from a raw DeviceAllocation is allowed only
while the owning native boundary proves that handle current; legacy raw handles
cannot distinguish an ID reused after reset. Later validation uses the captured
token. The future Harness snapshot must enforce its own owner lifetime before
calling these APIs; a token alone does not solve that boundary.

Retained metadata uses a separate accounted-capacity policy: the CPU query reports
`sizeof(CpuDevice)` once and the existing allocation table's capacity times its
extended AllocInfo size. The object includes new device-instance and epoch fields;
record rows include explicit live/import bits. No new per-token heap storage or
global allocation registry is introduced. The single common atomic identity counter
is process-global fixed support, not a per-device charge. Device capability-map
backing, allocator headers, process/JIT/driver/pool state and physical residency
are not implied by this narrow metadata report. Do not add the device object again
through another enclosing census. Queries allocate no storage and do not walk rows.

CPU allocation now unwinds its acquired payload if table registration throws;
the output handle is cleared. Existing CPU imports are explicitly non-owning on
release. Device copy is disabled so instance identity cannot be duplicated by a
copy. This slice does not change CPU destruction into a process/pool reclamation
policy; callers still follow native object/runtime teardown ordering. `clear()`
is an explicit quiescent resource operation, not an operation during a kernel or
while an owner still uses old handles.

`allocation_record_test` is registered in the native CTest suite (the hosted
round-robin selection includes new tests). It covers owned direct/runtime requests,
alias views, distinct equal-sized allocations, foreign devices, live/retired zero
requests, imported storage release/reset, retained metadata, allocation-free
inspection, ID reuse after clear, same-address device reconstruction, default
Unavailable behavior, and registration-allocation failure with retry. The CPU test
uses explicit CpuDevice construction; HCP model code adds no backend casts.

Remaining: native owner backing exports for tree/node/list and prepared-launch
families, complete trusted Harness coverage/lifetime, then combined accounted-byte
admission. Existing H/D and input/work gates remain unchanged. This contribution
does not qualify GPU allocation behavior or claim complete borrowed accounting.
## What the owner actually borrows

`ExchangeOwner` borrows one [Harness](../src/field/field.h), which owns the
prepared host structures, numerical arrays, activity and optional operation
components. The harness borrows [Runtime](../src/engine/runtime.h); Runtime owns
the Taichi Program. Neither the owner nor a request-supplied byte value owns that
backing. A Program can contain resources other than this harness's resources.

| Surface | Actual owning path | Present observation and missing coverage |
|---|---|---|
| Numerical arrays | Harness and Activity Ndarray objects | Ndarray exposes DeviceAllocation and logical shape, but a shape can be a non-owning view. Common Device has no live allocation extent query. |
| Prepared scratch arrays and host tables | [PreparedStorage](../src/field/prepared_storage.h), including geometry and inspection | Concrete members exist; `workspace_layout` is a requested layout, not a live request registry. Host vector capacity is observable but is a capacity charge, not independently recorded allocator requests. |
| Record-map backing | [PreparedRecordMap::Impl](../src/engine/record_map.cpp) | Owns an SNode tree, four Ndarrays, staged/returned host batches and bound contexts. `prepared_bytes()` reports context payload, not those other families. Native node pools are warmed by activation/deactivation and retain lists. |
| Optional operations | Activity, CentroidCascade, LiveHierarchy, TickExecution, Placement and nested AdmissionContacts/NativeSelectionWorkspace | Each owns buffers/contexts; binding-byte methods describe only selected payload. Enumerating PreparedStorage alone misses these owners. |
| Prepared invocation state | [CompiledKernel](../src/engine/runtime.cpp), LaunchContextBuilder and native launcher | `prepared_bytes()` sums argument/result buffers, selected prepared host/device bytes and vector capacities. It is not an identity-bearing request registry. |
| SNode root backing and native node pools | Native executor/tree buffer manager and runtime allocators | Root allocation identities are private in SNodeTreeBufferManager. Root storage and node allocator/list backing must be traced separately; no per-harness complete export exists. |
| Runtime-wide pools, compiler/JIT and driver resources | Runtime/Program and process/device pools | Not private to one harness. Must be explicitly excluded from the scoped accounted-byte policy or attributed once by a separate trusted owner. Never charge a whole pool and its suballocations as disjoint backing. |

The first five rows are required coverage work for a scoped prepared-execution
ledger. This table is an inventory, not proof that those rows have been measured.
Fixed host objects, nested allocations and reference aliases also require an
explicit capacity-vs-request convention. A field generation is not a backing
identity: ordinary updates change state without necessarily replacing storage.

## Why existing totals are not B

[PreparedCapacity::numerical_bytes](../src/field/prepared_capacity.cpp) sums
numerical shapes. [required_scratch](../src/field/prepared_storage.cpp) includes
workspace layout plus binding reserve. Harness setup also records actual binding
payload and unused reserve in its manifest. These describe different, overlapping
reservations; adding them and all component binding bytes duplicates coverage.
None supplies allocation identities, retirement validity or complete nested owner
coverage. Caller input C, snapshot S, bootstrap H, selected D and those layout
totals must not be relabeled as one disjoint request total.

The native source establishes additional distinctions:

- [Ndarray](../taichi/taichi/program/ndarray.cpp) constructed with Program requests
  `nelement * element_size`. Its DeviceAllocation constructors create views and
  do not allocate/free their backing. Summing array shapes counts aliases and can
  miss unused parts of an allocation.
- [DeviceAllocation](../taichi/taichi/rhi/public_device.h) contains device and
  numeric allocation ID, no extent, live bit or incarnation. Equality uses both
  device and ID. A pointer/ID tuple is meaningful only during its owning device
  lifetime; it is not a durable token across destruction/reset/reuse.
- CPU/CUDA/AMDGPU implement backend-specific `get_alloc_info`; the common
  [Device](../taichi/taichi/rhi/public_device.h) and
  [LlvmDevice](../taichi/taichi/rhi/llvm/llvm_device.h) interfaces do not expose a
  complete allocation-record query. HCP model code must not grow backend casts to
  guess this information. Unsupported coverage must remain explicit, not zero.
- [CPU](../taichi/taichi/rhi/cpu/cpu_device.cpp) records request size and allocates
  via HostMemoryPool. Its nonzero record retains size after release while its
  pointer becomes null. A zero-size request can also have a null pointer, so
  pointer-null alone cannot be the proposed general lifecycle contract.
- [CUDA](../taichi/taichi/rhi/cuda/cuda_device.cpp) and
  [AMDGPU](../taichi/taichi/rhi/amdgpu/amdgpu_device.cpp) runtime paths record a
  page-rounded size. Requested array payload, backend-rounded allocation extent,
  pool reservation and physical residency are different units. The audit does not
  claim GPU execution or decide these units are interchangeable.
- [LlvmRuntimeExecutor](../taichi/taichi/runtime/llvm/llvm_runtime_executor.cpp)
  records runtime DeviceAllocations, but Ndarray destruction calls Device
  deallocation directly. The executor's map is therefore not by itself a live
  registry; finalization explicitly checks whether recorded memory remains live.
- [SNodeTreeBufferManager](../taichi/taichi/runtime/llvm/snode_tree_buffer_manager.h)
  privately maps tree IDs to root DeviceAllocations. That map is not the native
  node/list allocator ledger and is not exported through PreparedRecordMap.
- [Prepared CUDA launch buffers](../taichi/taichi/runtime/cuda/kernel_launcher.cpp)
  use Device allocation directly, outside the executor runtime-allocation map.
  Their identities are held in the private PreparedCUDA subtype. Its byte totals
  alone do not expose alias, owner or retirement information.

Thus neither walking Ndarray shapes nor summing the executor map establishes B.
The gap is an owner-scoped native request/lifecycle interface and coverage of
SNode/node-pool and prepared-launch paths, not a missing arithmetic addition.

## Bounded implementation sequence

This sequence is proposed implementation detail for review, not built behavior.
The existing accepted requirement remains a trusted, disjoint ledger.

1. Add a native Taichi allocation-record query at the common device ownership
   boundary. It must distinguish original requested extent from any backend
   rounded extent, report live/retired/unknown status, and carry validity tied to
   the owning device lifetime and allocation incarnation. Cover direct and
   runtime allocation plus release/reset/import paths; an import needs its own
   explicit backing ownership classification. HCP code consumes one interface.
2. Expose the actual backing references through their owners: array allocations,
   tree roots and node/list reservations, prepared launcher buffers. Establish
   ownership/coverage for the concrete Harness component table above. A complete
   result cannot be synthesized if one required family is unavailable. Do not
   replace native node-list requests with padded logical record capacity.
3. Assemble a trusted Harness snapshot under its exclusive boundary, valid only
   while that harness/runtime/backing set remains alive and unchanged. It must
   distinguish same-allocation aliases from same-sized distinct allocations and
   resources in different runtimes. Store backing validity separately from normal
   publication generation. Do not accept caller-authored amounts or tokens as
   authority; copying a snapshot must not extend native lifetime silently.
4. State the chosen accounted-byte policy for host capacities and native request
   extents. Enumerate exclusions: shared runtime pools/JIT/driver/process overhead
   and physical residency are not covered merely because a prepared ledger exists.
   Borrowed host/native request families are disjoint from C/H/D; ambiguous aliases
   must be resolved at the owning boundary, not counted twice or dropped.
5. Only then enforce checked C+B+H+D(t), retaining separate work limits. Validate
   the ledger at admission and across backing replacement; reject incomplete,
   foreign or expired evidence before mutation. Fixed B permits peak arithmetic;
   changing backing requires simultaneous accounting, not summed independent peaks.

Acceptance controls must cover empty/zero-sized and aligned/rounded allocations,
same allocation through multiple views, distinct equal-sized allocations, foreign
runtime/device, release/reset/reuse, partial constructor unwind, checked overflow,
incomplete coverage and exact retry. Owner tests retain native-state preservation
and every-allocation-site reclamation. CPU tests do not qualify GPU paths; future
backend qualification must use native mechanisms on the actual target hardware.

## Focused executable evidence

A new local Clang15 CPU-only probe against reviewed `5771326e` directly used
CpuDevice and Ndarray's borrowed constructors. It allocated 64 bytes, constructed
64-byte and 16-byte views of that same allocation, and observed a view sum of 80
against one 64-byte backing record. View destruction left backing live. A second
CpuDevice issued the same numeric ID, but DeviceAllocation equality distinguished
the devices. After nonzero release, the first record still reported size 64 and a
null pointer. All assertions passed; this is evidence of the specific hazards,
not a native ledger implementation or a full-owner accounting measurement.

The diagnostic's explicit CPU cast is confined to local test code. It is not a
proposal for backend-specific HCP model logic. No Runtime, model, GPU, database,
browser or accepted native matrix was run. No source/build/test failure occurred.

Verified local archive `setup-evidence/borrowed-native-audit-5771326e.tar.gz` retains
probe source, exact compile/link templates and commands, binary, result and source
pins. SHA256SUMS manifest:

    db36ea71dc6dd15fb6e4e6adef25c28ac5c494409633d573bcdb9b137fb0a32b

Raw local evidence has not been independently inspected remotely. This audit
changes no runtime or tests. #298/#299 remain provisionally accepted, with their
hosted native runs still active at review; no duplicate run or merge is initiated.
