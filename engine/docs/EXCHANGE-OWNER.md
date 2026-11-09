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

Hierarchy sealing now takes explicit per-call payload-byte and copy-work
allowances. The checked byte charge combines the caller's three table spans with
the three retained destination capacities; copy work is one unit per payload
byte. Exhaustion or arithmetic overflow returns before source table access,
leaving the unsealed pending operation available for retry. Existing native
capacity and malformed-input checks still apply before copying. This is a
bounded sealing gate, not a total owner memory/preparation-work policy; see
[the precise budget scope](OWNER-RESOURCES.md#hierarchy-sealing-gate-contribution-awaiting-review).

Adoption also retains two immutable exchange declarations per selected occurrence,
one for each direction. Both come from the validated structural recipe: the reverse
swaps outgoing/incoming descriptors and from/to exposure, retaining the same exact
mass, ordered leaf inventory and symmetric comparison-work bound. Each declaration
keeps the full study/occurrence key, pinned recipe and definition revisions. Copies
of the same letter have distinct declarations; no live identity is deduplicated.

Preparation selects by exact owner key and target and verifies the declared source
exposure against the current projection. Foreign studies, unknown entries, nested
occurrences, invalid targets and requests for the current exposure reject before
binding recapture. The binding shares the selected const declaration without
copying its nested descriptors. Readiness/generation and current source participation
are still validated afresh; a retained declaration is not live execution authority.
There is no mutable declaration alias on this path. Reference-counted ownership
keeps it alive through binding teardown; all harness/owner calls remain exclusive.

Owner preparation now requires an explicit allowance for binding source capture.
Capture charges structural row work, variable-length identity comparisons, mask
fills and final validation before performing them. Exhaustion/overflow returns
with an invalid binding and no pending owner operation; partial candidate buffers
are overwritten by retry. Committed identity, reports and native state are unchanged.
The separate staging gate now covers declaration selection and projection/map staging. Later
command construction now has its own allowance. See [capture-work scope](OWNER-RESOURCES.md#source-capture-work-gate-contribution-awaiting-review),
including setup's separate allowance and exception-runtime accounting limits.

Preparation returns separate staging/capture/command results and a phase identifying
where it stopped, or `Complete`. Command-work exhaustion leaves the coordinator
consumed/unsealed and the owner non-pending, even though captured source metadata
may be valid. The candidate's partial commands/maps are overwritten on retry;
committed owner/native state is unchanged. Command searches, copies, source
revalidation and field-retirement moves are charged before their work. See
[command-work scope](OWNER-RESOURCES.md#command-construction-work-gate-contribution-awaiting-review).

The same lifetime holds a membership catalogue for all declared alternatives.
It follows only pinned upward memberships of the owning letter, preserving each
occurrence's identity, inherited group revisions and exact source mass. Binding
capture filters outgoing occurrences; command generation filters incoming ones
in the existing trusted field order. No inactive catalogue row is materialized as
live participation. The catalogue rejects ordinary live binding, and the public
membership function still enumerates only active occurrences. Study and recipe
pins are checked before source capture.

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

### Combined retained-capacity report (accepted #231)

`ExchangeOwner::storage()` now reports both projections, all six nested map stores,
binding and coordinator objects/buffers, both declarations per selection, and the
all-alternative membership catalogue. Shared pointees are counted once; control
block identities are reported separately from their unmeasured byte overhead.
The checked total uses the existing conservative capacity convention, including
constructed inactive map slots and the field-command vector. Reporting does not
allocate. This is a retained payload-capacity charge, not a complete memory budget.

The [owner resource inventory](OWNER-RESOURCES.md) specifies every charged family,
ownership/cleanup paths, adoption overlaps, borrowed native/caller surfaces and
the work still outside execution's existing allowances. Complete byte/work budget enforcement remains next. Accepted #232 supplies scoped adoption allocation-peak,
every-allocation failure and final-alias reclamation witnesses; their exact
coverage and exclusions are in that inventory. These do not establish a total
byte/work gate. The projection-only figures below retain their original scope.

### Retained nested maps (accepted #228)

The owner, binding and coordinator each retain an occurrence map and a field map.
Each `ExchangeMap` separates the active length from constructed reserved slots.
Adoption reserves occurrence slots to `Particles + Batch` and field slots to
`Fields + min(Fields, Batch)`, with checked sums. These include candidate metadata
that may exceed native live capacity: preparation must still describe an oversized
request so native preflight can report ordinary `CapacityExhausted`. Reserving host
candidate keys does not increase native claim limits. The same capacity is retained
in each store because publication rotates their roles.
Every slot has UUID storage and a path/address buffer sized from the full pinned
alternative inventory, including trusted initial field identities. These are host
identity slots only: unused slots do not become native objects, claims or mass.
The coordinator reserves the parallel field-command index vector to the same
candidate field bound.

Staging copy-assigns keys into existing slots. Clearing changes only the active
length; field retirement rotates whole slots, preserving live order and nested
storage. Incoming keys are assigned by reference without temporary key objects.
Slot or nested-key exhaustion rejects before the offending append and cannot grow
backing. A failed staging operation may leave partial candidate metadata, but the
binding/owner pending gates are already invalid and committed mappings remain
unchanged. Valid retry overwrites the candidate. Publication swaps complete map
stores and their active lengths with no allocation. Copies made during standalone
adapter setup preserve reservation even for inactive slots.

The first and repeated successful `ExchangeOwner::prepare` calls are now guarded
against C++ `new`, including successful retries after partial preparation/capture
failure. Both owner sequences also compare the complete pool of outer map and
nested path/address buffer addresses and capacities across publication swaps.
Map tests cover erasure order, full capacity, oversized keys, empty-copy reservation
and retry. An oversized incoming field key cancels pending intent without growing
the pools or changing committed state. The independent four-copy sequence retains
its shared fields and survivor-history checks.

This reservation has a real setup and lifetime cost: three constructed occurrence
stores, three constructed field stores, all their nested UUID/path/address backing,
and the field-command vector survive until teardown. They are **not included** in
`projection_storage()`. The pointer/capacity checks are reuse witnesses, not a total
byte census or budget. Complete owner peak/lifetime/work accounting remains next,
including these slots, immutable declarations/catalogue, setup transients, shared
ownership overhead and borrowed native surfaces. Caller-built inputs and hierarchy,
adoption, standalone adapter construction and exception construction may allocate.
No allocator/RSS, GPU, performance or other-operation qualification follows.

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

The coordinator also retains one [binding](EXCHANGE-BINDINGS.md) and its reserved
mass, outgoing-handle, source-edge and two mask vectors. Preparation assigns the
current complete maps and captures source data afresh into those buffers. The
binding is invalidated before fallible work and becomes valid only after full
capture and validation. A failed capture never leaves a partially refreshed view
usable; successful owner/native publication continues through the same swaps.

This moves these allocations to adoption rather than eliminating their cost.
Their reserved payload is the sum of each vector's capacity times its element
size; it is **not included** in `projection_storage()`. Commands and hierarchy
remain bounded by the borrowed harness's capacity; this is not a separate total
owner byte-budget admission gate. Coordinator and binding objects remain resident
until teardown; invalid preparation may leave unusable partial metadata resident.
Declaration construction now occurs at adoption; successful selection and sharing
perform no C++ allocation. Two declarations per selection, their nested descriptors,
table capacity and reference-count control blocks remain resident until their last
owner/binding reference is released. These surfaces are **also excluded** from
`projection_storage()`. Adoption copies the reverse alternative and still has
setup transients; no complete peak/lifetime or byte-budget claim follows.
Membership traversal now occurs at adoption and the catalogue is shared across
preparations. After nested occurrence/field maps are staged, fresh source capture
uses reserved buffers and borrowed key references without C++ allocation. Nested
map staging now reuses retained slots; caller preparation still allocates. The
catalogue, its nested rows/pins and ownership control block, and the binding buffers
are all outside the projection charge. The combined report now counts retained
payload capacity; control-block bytes remain outside that charge.
Setup transients, native storage and lifetime/cleanup measurement also remain outside
the projection charge.
Successful private owner preparation is guarded against C++ `new` in the tested
sequences; complete peak/lifetime accounting remains open. No new native numerical
kernel or backing array is introduced.

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

C++ `new` guards cover the first and repeated source-capture, declaration-selection and projection-staging calls as well
as hierarchy sealing from its first call, successful execution, ordinary rejection
and ticks. Tests retain the coordinator address and all five buffer addresses and
capacities across repeated edits. A malformed incoming intrinsic forces failure
after binding recapture/partial plan construction, invalidates preceding pending
intent, preserves the committed owner and permits a valid retry. Both view addresses and
the combined projection charge remain stable; tests include active cardinality
changes, overflow rejection, and an isolated address-reservation increase with an
exact expected charge delta. Caller preparation, adoption and exception construction
remain excluded. Native backing is observed separately.

Declaration tests retain all two/eight original declaration identities across the
single/four-occurrence sequences, compare selected descriptors (including ordered
leaves and comparison work) against fresh derivation in both directions, and verify
that the actual binding holds the selected object rather than a copy. Invalid-key
preparation cancels pending intent without changing the committed owner and permits
a valid retry. Mixed initial aggregate/parts exposure is covered by the four-copy case.

The membership value test compares a retained all-alternative catalogue with fresh
live derivation across 32 exposure combinations, repeated copies, different language
contexts sharing byte definitions, multi-byte/repeated characters and shared ancestor
paths. It checks exact source paths, pins and counts while retaining catalogue
backing, and rejects use of that catalogue as an ordinary live binding.

[Local evidence](../tests/admission/README.md) records source and configurations.
This CPU scope does not qualify GPU behavior, all operation rows, full owner
resources, browser control or the integrated local study.

The staging-work contribution adds an explicit allowance before capture and command
construction, with a separate result and stopping phase. It gates declaration
selection, projection flag/parent-pool updates and initial binding maps. Complete
parent-pool transfers are prepaid so exhaustion cannot strand moved-from rows;
retry rebuilds partial metadata without publishing it. See the
[charge boundaries and limits](OWNER-RESOURCES.md#owner-staging-work-gate-contribution-awaiting-review).
Adoption and combined memory policy remain separate outstanding work.

Adoption now enters through a const-reference factory with explicit input byte,
inspection-work and copy-work allowances before owner allocation or argument
copies. Budget rejection returns no owner; exact input admission permits the
existing constructor and its unwind behavior. This gates only caller/input-snapshot
payload, not later constructor allocations or complete setup work. See
[input admission scope](OWNER-RESOURCES.md#adoption-input-snapshot-admission-contribution-awaiting-review).

The constructor-buffer contribution adds a separate allocation domain for all
binding capture/mask buffers (including temporary validators) and coordinator
command/hierarchy buffers. It admits actual requested bytes before allocation,
tracks live/peak bytes and cumulative request work, and survives constructor unwind
so budget rejection can report full reclamation. Other constructor allocation and
work domains remain open. See [allocator scope](OWNER-RESOURCES.md#bindingcoordinator-allocation-domain-contribution-awaiting-review).

The map-store contribution extends that allocation domain to all six retained map
outer vectors, initial owner map copies and explicit temporary validation/staging
maps. Ordinary copied snapshots detach from the source owner's accounting lifetime;
setup-owned copies explicitly attach. Assignment retains the destination domain,
and swaps move it with the backing. Separately allocated nested key payload remains
outside this limit. See [map-store scope](OWNER-RESOURCES.md#map-outer-store-allocation-domain-contribution-awaiting-review).

The shared-object contribution extends allocation admission to declaration-table
backing and declaration/catalogue object/control-block requests. Their allocators
retain the accounting domain through final shared or weak release, including after
owner destruction. Independent table snapshots detach their backing allocator while
retaining shared pointee ownership. Nested payload and preceding construction/graph
work remain separately scoped. See [shared allocation lifetime and limits](OWNER-RESOURCES.md#shared-object-allocation-domain-contribution-awaiting-review).
