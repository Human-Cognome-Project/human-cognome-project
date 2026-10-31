# Contact-safe candidate ticks — B1 integration

Status: implemented and locally CPU-qualified for review in #201 under the accepted
[#199 plan](../../docs/native-admission-contact-transaction-plan.md), following
#200's accepted [native selection qualification](NATIVE-SELECTION.md).
The [source-pinned record](../tests/contact_ticks/v1/README.md) at `3cabc70`
passes 109 CPU suites, all eight B1 modes, 544 new LLVM modules and 180
PostgreSQL integration checks. Maintainer acceptance and hosted CPU validation
remain pending. Placement (B2), bootstrap/admission (B3), enduring
owners and CUDA remain separate. This does not close #156/#172/#177/#145.

A nonzero `Budget::TickWork` opts a prepared f64/x64 owner into
`Harness::tick_prepared(expected_generation, Parameters64, tick_work,
geometry_work)`. GeometryWork is also required. A missing optional call limit
uses its prepared reservation; a larger call limit rejects. Disabled owners
retain their existing numerical layouts and signatures and return Unsupported.
Enabled owners refuse direct `tick64`, which would bypass contact validation.
An empty prepared owner remains valid; sentinel rows are not physical objects.

## Ownership and execution

The Harness exclusively owns private numerical/control surfaces, two private
geometry views, an installed physical contact union and fixed bindings for shared
production field, Activity and installed C3 builders. Presence, source
relationships and immutable hierarchy descriptors are borrowed from the same
Runtime. The private constructor checks Runtime, exact logical capacities,
profile and original bindings before constructing the selection workspace.
Member lifetime keeps every borrowed owner alive until its kernels are destroyed.
Only the committed native type sets are authoritative between attempts.

Each attempt stages numerical/control state by native copies and copies native
membership into prepared scratch selections. Numerical phases still use native
`struct_for`, activation/deactivation and list generation. This adds no physical
particle population, host numerical replay, queue or awake-array scheduler.
Current coverage invalidation occurs before physics. The shared field/C3 force,
contact, one integration, contact propagation and centroid poll keep their
existing ordering and equations. C3 retains full same-poll centroid work,
at-most-one visitation and late pending notifications. Positive dt retains
accepted displacement/dt; zero dt preserves incoming velocity under #196.

Both geometry boundaries reuse the existing native collection, explicit
source-order, discovery and CSR builders. Discovery includes present sleepers
and uses actual published coordinates. Unique explicit pairs lead in original
source order/orientation; derived-only pairs follow in canonical order.
The explicit ledger is never replaced by the physical union. Private geometry
outputs are cleared before discovery so rejected scratch tails cannot enter a
later publication. Current CSR feeds forces and propagation; post-motion
coverage changes wake affected present endpoints for the following particle phase.
The public observation query retains its separate workspace and tokens.

After all checks succeed, native copies publish numerical/control values and
the complete union/mapping/CSR, then native membership. The owner synchronizes
before exposing a successful report. Changed union or mapping advances one
prepared generation and retags installed source stamps without reseeding
packets/history. Unchanged coverage retains generation. Every accepted tick
advances completed-tick observation, invalidating prior geometry query tokens.
Changed generation invalidates source/capture mappings; a stable mapping remains
usable to observe subsequent completed ticks.
`TickResult.current/candidate` are attempt diagnostics with query zero, not
public inspection tokens.

## Rejection and failure

TickWork, GeometryWork, Contacts and References exhaustion return a named typed
failure before committed copying. Both geometry boundaries share one allowance.
Reference peaks include the explicit source ledger, installed hierarchy and all
simultaneously retained installed/current/candidate physical pairs. Parameter,
stale-generation and candidate numerical/domain checks precede publication.

Healthy rejection preserves body/field values, speeds, displacement/motion,
packets/selectors/history, focal/universal controls, native bits, source/union/CSR,
parameters/invalidation, frame, generation, timings, completed ticks, previous
Demand and observation bindings. Attempt demand remains separate from
`Harness::demand()`. Scratch may contain abandoned values; the next attempt
restages it. No committed-state restore obtains this guarantee.

An unexpected backend exception makes execution unready, including an exception
before publication when candidate completion is no longer trustworthy. Failure
after committed copying begins never claims healthy rollback. Destruction stays
valid. Test-only injections distinguish candidate failure from partial publication.

## Reservations and measurement

For physical sentinel strides p/g/h, the extra f64 candidate subtotal is
`8*(15*p + 14*g + 12 + 12*h)` bytes, with
`4*(2*p + 2*g + h)` bytes for flags/selectors. The h terms apply only with the
live hierarchy enabled. Add 20 i64 Activity counters and, with C3, 10 i64 counters
and 2*h i64 visits. Two complete private geometry views, the installed union and
converted force CSR add `4*(22*p + 34*c)` bytes, 48 i64 geometry counters and
8 f64 focal values. Validation adds two i64 values. These surfaces overlap during
validation and are reserved simultaneously.

The manifest also inventories fixed owner/geometry/array control objects, copy
binding descriptors and prepared context payload. The extra binding reservation
is 531 parameters/34 contexts with C3, or 417/31 without C3. Static
`required_scratch` is a preflight lower requirement: setup additionally measures
and charges the actual native requested-byte delta for scratch masks/list
high-water before returning Ready. A static-only reservation can therefore fail
setup with ScratchBytes. This is not whole-process heap or driver residency
accounting. Candidate storage and work scale with prepared P/G/E/C/h capacities,
not potential address V.

TickWork units are conservative scalar-copy slots and bounded source/reference
row evaluations, not FLOPs. Numerical accounting includes nested C3 recipient
checks; overflow saturates and rejects. GeometryWork separately records pair,
source-order and union-order comparisons. Results also expose copy slots,
numerical/selection bounds, geometry work, synchronization and status readback.
Setup compiles, binds, provisions and warms enabled families and unused blocks.
CPU transfer surfaces are ordinary owned allocations; they do not establish GPU
pinning or arbitrary external buffer lifetime safety.

## Qualification

`live_hierarchy_test --case contact_tick` exercises shared-builder parity,
positive/zero dt, dirty multilevel packets/history and pending lag, moving contact
overflow and late work/reference rejection, independent no-attempt retries,
source orientation/duplicates, contact entry/exit and both CSR force directions,
full committed snapshots, stale observations and failure disposition. Retained
letter/UTF-8 intrinsic fixtures exercise 32 reject/commit cycles. Asymmetric
P/G capacities 70/4096 exercise last records and unused pointer blocks with three
present bodies/fields; they are not a dense 4096-body numerical qualification.

Actual emitted modules are audited for allowed ndarray writers and owned native
selection roots, SC mask operations, paired copy rows and enable guards, with
LLVM-verified unsafe mutations and safe redundant stores. The actual candidate
C3 force/poll retain the existing publication/history/semantic controls. Frozen
stage/publication bindings are checked against owned, disjoint buffers. These
are bounded argument/root/row checks, not a general alias/equivalence proof;
runtime allocator/list bookkeeping outside proven model roots stays distinct.

The compiler collector retains 31 candidate modules plus three selection modules
for each of two capacity/precision profiles, across one/two workers,
BulkCopy/CpuDirect and default/no-unroll LLVM shapes: 544 modules. Existing C3,
refresh and selection gates remain enabled. Final source-pinned regression,
compiler matrix, resource observations and read-only-cold PostgreSQL integration
are retained in the [review evidence](../tests/contact_ticks/v1/README.md).
