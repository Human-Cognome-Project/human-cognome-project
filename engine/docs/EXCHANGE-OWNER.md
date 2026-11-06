# Persistent private exchange owner

`assembly::detail::ExchangeOwner` retains one logical study across successive
aggregate/parts exchanges on a borrowed prepared `field::Harness`. The harness
must outlive the owner, and all owner and harness calls must be exclusive. This
is a private exchange-lifecycle increment of continuation slice C, not the full
operational owner or a public Add/control API.

## Identity and live projection

Adoption takes a structural-v2 `LetterProjection` and complete trusted occurrence
and field mappings from actual admission claims. It validates current handles,
exact masses and every selected occurrence's participation against the prepared
execution. It cannot infer logical identity from numeric values or certify a
caller-supplied layout association. The private adapter retains that trust boundary.

The study UUID, occurrence keys, definitions and pinned recipe remain stable.
`LetterProjection::recipe()` still returns the immutable initial recipe; its
selection exposure is not current live state. `exposure(entry)` returns current
exposure. Active occurrence flags, parent contributions and the structural binding's
selection exposure follow that live view. Inherited memberships derived from the
projection consequently follow the current alternative and exact source masses.
Exchange declarations use the current exposure while their alternatives still
come from the immutable recipe. No new projection UUID is minted between exchanges.

## Preparation and publication

Adoption reserves two projection views and one reusable
[coordinator](ATOMIC-EXCHANGE.md). Preparation updates the spare view and
rebuilds the coordinator's generation-bound intent using the owner's current
complete maps. Each view retains every possible parent contribution
across the pinned alternatives, partitioned between active and spare vectors.
Changing exposure moves these existing objects, including their nested strings and
paths, rather than destroying and recreating them. Active rows retain canonical
occurrence/source-parent order. All selections and active flags are refreshed from
the committed view before applying the requested change; a spare view may be older
than the preceding operation when multiple occurrences are edited in succession.
Incoming layout/geometry bindings and the prospective hierarchy are still supplied
by trusted preparation. Preparation and hierarchy sealing do not publish any live
projection or mapping. A new preparation discards previous pending intent; a
preparation exception preserves committed identity, mappings and last report.

Execution reserves 64 PlacementWork units for owner publication before delegating
the remaining work to the existing coordinator. The native transaction still
validates old-plus-new claims, conservation, placement, source, contacts and
hierarchy. After its success, no-throw vector and pointer swaps publish occurrence
and field mappings with the prospective projection, followed by fixed-size report
and generation assignments. All publication is inside the same exclusive boundary.
There is no intermediate healthy owner visible to another caller.

Ordinary rejection leaves the committed owner and execution unchanged and permits
retry of the same prepared operation. Success consumes that plan; the owner can
prepare another operation against the newly committed projection and actual native
handles. Last reports remain descriptions of a completed exchange, never numerical
input and never a claim of current state after ticks. An unexpected failure after
native mutation, including injected failure point 8 before owner publication,
leaves the harness unready and the previous owner snapshot intact. This is not
rollback or recovery of a partially published execution.

Ticks with unchanged source generation remain compatible. An unrelated operation
that advances the source generation makes this owner stale; it does not silently
rebind. Integrating admission, removal/reuse, focus/rebase and restart through the
same owner is subsequent work.

## Resource boundary and evidence

Projection reservation occurs at adoption: both complete views and their immutable
recipe copies remain allocated until owner teardown. Repeated projection staging
does not copy the recipe or allocate nested parent storage. After publication the
previous committed view becomes the spare, to be overwritten on the next
preparation; references to an old view do not remain current or immutable across
that preparation. Initial reservation still allocates and copies host data.

`projection_storage()` reports both views' fixed object bytes, vector capacities
(including unused element slots, object addresses and occurrence paths), string capacities plus
terminators (including UUIDs), and constructed active-plus-spare parent rows.
`charged_bytes()` is their checked sum. String capacity is conservatively charged
even when held inline within an already-counted object. Thus this is a retained
capacity charge, **not** measured heap allocation, allocator overhead, RSS, driver
memory or a provisioning peak. Both parent vectors reserve the full alternative
inventory per view; their unused raw slots are counted as vector capacity, while
nested storage is counted only for constructed rows. Aggregate overflow rejects
without publishing a partial report. The census and projection staging do not
enforce a complete owner memory budget or reserve preparation-work units.

The coordinator reserves command and intrinsic vectors to `Batch`, and hierarchy
node/child/contribution vectors to `CascadeNodes`, `CascadeLinks` and
`CascadeReferences`, respectively, at adoption. It retains these five buffers
until teardown, including across failed preparations. Every command append checks
the batch limit before writing; an over-budget plan cannot grow command backing
before rejection. Hierarchy sealing checks all three limits before copying and
uses the reserved buffers from its first call. Each preparation clears all five
logical lengths, including empty hierarchy tables, and invalidates previous
pending intent before any fallible work. Only a fully prepared plan becomes
pending again. The stable coordinator address is not a persistent transaction
identity: its views describe the most recently prepared intent and are replaced
on preparation. Successful native publication still uses the existing swaps.

This moves these allocations to adoption rather than eliminating their cost.
Their reserved payload is the sum of each vector's capacity times its element
size; it is **not included** in `projection_storage()`. Commands and hierarchy
remain bounded by the borrowed harness's capacity; this is not a separate total
owner byte-budget admission gate. The coordinator object and old bindings can
remain resident after invalid preparation until replacement or owner teardown.
Declaration/binding construction, membership enumeration, nested occurrence/field
map storage and caller inputs still allocate. Setup transients, native storage
and lifetime/cleanup measurement also remain outside the projection charge.
Allocation-free **whole-owner** preparation and complete peak/lifetime accounting
remain unqualified. No new native numerical kernel or backing array is introduced.

`exchange_owner`, also included in the combined placement matrix, alternates 32
exchanges of one two-byte letter on one execution, with a tick after each. It checks
stable identity/immutable recipe, current structural exposure and contributions,
actual handles, retained focus, report/mapping publication, consumed-plan rejection,
unchanged stale/work/ordinary rejection and retry, and late unready behavior.
`exchange_owner_multiple` adds four copies of the same definition. It alternates
edits between two of them for 32 exchanges/ticks, checking previously edited and
untouched copies, surviving numerical state/handles, shared fields/focus, exact
parent/membership inventory and unchanged rejection/retry. The two permanent
copies keep both field alternatives in use; the original single-occurrence test
continues to exercise field creation and retirement.

C++ `new` guards now cover the first and repeated projection-staging calls as well
as hierarchy sealing from its first call, successful execution, ordinary rejection
and ticks. Tests retain the coordinator address and all five buffer addresses and
capacities across repeated edits. A malformed incoming intrinsic forces failure
after binding creation/partial plan construction, invalidates preceding pending
intent, preserves the committed owner and permits a valid retry. Both view addresses and
the combined projection charge remain stable; tests include active cardinality
changes, overflow rejection, and an isolated address-reservation increase with an
exact expected charge delta. Other host preparation and exception construction
remain excluded. Native backing is observed separately.

[Local evidence](../tests/admission/README.md) records source and configurations.
This CPU scope does not qualify GPU behavior, all operation rows, full owner
resources, browser control or the integrated local study.
