# Adoption accounting inventory before a combined limit

Status: source audit for review, 2026-11-12. This document closes the bounded
inventory requested by [#295 review 5877025531](https://github.com/Human-Cognome-Project/human-cognome-project/pull/295#pullrequestreview-5877025531).
It does not implement or accept a combined admission limit. Audited runtime:
`803437be28971c9efcd80ded72c2eae3a05eecae`, unchanged through reviewed #295
`c118e6efea6902a3996af126e67840f5c4e7da1a`. Main `5ef632f2` adds current handoff
notes; normal merge `46e7d1ad` preserves them in this continuation.

## Checked map prepayment implementation (awaiting review)

Following #296 review 5877302279, input admission now computes separate
`projection_copy_work`, `occurrence_copy_work`, `field_copy_work` and
`required_copy_work` values. The last is the checked sum P+M_occurrences+M_fields;
it replaces the former copy-work charge S. Caller/snapshot/required **byte** values
remain C, S and C+S. Demand arithmetic is part of allocation-free inspection; only
a complete inspection publishes the breakdown. Byte refusal still precedes the
single copy-work charge. Zero/short work refuses with zero charged copy work and
the full required amount, before owner/domain allocation.

The initial map constructors intentionally still omit the setup meter: their
fresh construction demand has already been prepaid here. Capacity slack remains
caller occupancy only. Payload growth changes just its owning component; an empty
map costs one resize admission, not its old vector-header snapshot charge. No
unsigned subtraction or duplicated snapshot work is used. The implementation
checks each multiplication/addition rather than first forming unchecked 3*t or
an unsigned M-S. Its helper is for fresh maps and nonzero sizeof(T), not growth.

Fixed-work policy: the existing initial admission unit covers bounded owner/domain
header, scalar and control initialization; membership's fixed admission covers
bounded catalogue header/scalar construction and publication control. Existing
fixed metadata inspection charges include bounded checked arithmetic. These are
logical fixed-operation units, not byte-for-byte construction estimates. They do
not cover input-sized scans/copies/reservations, native computation, opaque allocator
runtime or physical memory. Existing size-based conservative charges remain where
already specified; no arbitrary new sizeof-based fee is added. Domain bootstrap
**request bytes** and a trusted disjoint borrowed ledger remain the next work.

The historical inventory/formulas below describe the reviewed #295 boundary;
this section supersedes its proposed map-work replacement and open fixed-fee choice.
Combined admission is still not implemented or accepted.

## Counters have different meanings

| Counter | What it measures | Do not add it to |
|---|---|---|
| `AdoptionInputResult.caller_bytes` (C) | Conservative capacity census of the supplied projection and two binding vectors, including headers and nested values | A second census of those same caller objects |
| `snapshot_bytes` (S) | Logical-size payload census of one projection and two row tables; excludes caller capacity slack | Selected-domain live/peak bytes or retained owner capacity as an independent allocation |
| `required_bytes` | C + S, an input-payload gate before construction | A combined requested-allocation peak; C/S are not allocator requests |
| `copy.charged` | Exactly S logical work units prepaid once | Another full charge for the same initial copy without replacing or crediting this component |
| `setup.work` | Separate fixed, scan, comparison, copy, reservation and relocation units | Allocation work or bytes as interchangeable units |
| `ReservationResult.live_bytes/peak_bytes` (D) | Live / maximum simultaneous successful allocator requests in the selected owner domain | Cumulative allocation work, S, or `OwnerStorage::total()` |
| `charged_work` | Cumulative admitted request bytes, including admission preceding an underlying allocator failure; releases do not refund it | Simultaneous live resources |
| `OwnerStorage::total()` | Conservative retained-capacity census, including embedded objects and the domain object but not its control-block bytes | D or S; most of its backing is already in D |

`ReservationDomain::before` checks multiplication, live addition and cumulative
addition before allocating. Failed byte/work admission changes neither live nor
charged work; an underlying allocation failure after successful admission keeps
its work charge but acquires no bytes. Peak increases only on successful acquire.
These are requested allocations, not allocator metadata, physical residency or RSS.
See [the allocator](../../common/reservation_allocator.h),
[input gate](../src/assembly/adoption_input.h),
[projection census](../src/assembly/projection_storage.cpp) and
[retained census](../src/assembly/owner_storage.cpp).

## Adoption operation coverage

The rows follow [ExchangeOwner::adopt and its constructor](../src/assembly/exchange_owner.cpp).
All mentions of selected requests include their independent byte and cumulative
allocation-work checks. Existing constructor unwind releases partial selected
storage; caller inputs and native execution remain borrowed.

| Operation | Work / input gate | Request gate and remaining distinction |
|---|---|---|
| Inspect caller and logical snapshot | Allocation-free inspection meter; checked C/S sum; input bytes then copy-work gate | No owner allocation before these gates. Scalar/header census work is not payload-copy execution. |
| Create reservation domain | Initial setup charge 1 precedes creation | `make_shared<ReservationDomain>` is outside its own quota and outside the constructor catch. Object/control-block request is excluded; underlying failure may throw. |
| Allocate and construct owner | Same fixed admission precedes it; no separate `sizeof(ExchangeOwner)` setup charge | `reserve_unique` admits the complete owner object request. Embedded map/vector/smart-pointer headers must not also become separate byte charges. |
| Copy initial projection | Logical projection component P of S prepays one copy, including the projection object's census | Its object, recipe/input/output tables and nested payload requests use the selected domain. This is the retained committed projection, not an additional detached argument snapshot. |
| Construct initial `occurrences_` and `fields_` | Their logical components of S prepay one row representation; constructor still omits the optional map meter | Outer slots, nil UUIDs and assigned identities are selected-domain allocations. Exact per-operation work differs from the old prepayment; see derivation below. |
| Ready/version/inventory checks | Fixed constructor control; no size-dependent search here | No native mutation or additional backing. Fixed-unit policy must remain explicit, not presented as bytes. |
| Collect membership catalogue | Membership meter charges fixed admission, pin copies, graph traversal/comparison, frames and output/relocation | Graph/output backing selected. `allocate_shared` uses the selected allocator for the actual rebound shared request. Fixed shared-object publication has no distinct `sizeof(MembershipFields)` setup fee. |
| Collect/copy declarations and publish table | Declarations meter covers lookups, payloads, row growth, reverse copy and fixed shared-object/table work | Actual shared object/control-block and vector requests selected. Binding aliases are not additional pointees. |
| Validate each temporary binding | Bindings meter covers buffers, packed-mask initialization, trusted/staged maps, staging and adoption capture | All temporary backing selected and released before leaving its loop iteration. It overlaps retained projection/catalogue/declarations while alive. |
| Inspect/copy spare projection | Separate inspection and logical-copy charge in setup meter | A second real projection exists, so its selected requests are distinct. Do not add its logical copy charge to D as bytes. |
| Reserve both exposure pools | CommittedParents / SpareParents charge scans, equality, slots and nested copies | Both projections' active/spare backing selected, including growth overlap. |
| Construct retained coordinator and binding | Bindings charges fixed objects, buffer reservations and mask initialization | Unique objects and all reserved backing selected. |
| Reserve owner/binding/coordinator maps | Bindings charges inventory scans, slot growth, nil initialization and nested capacity | Six retained map stores selected, including inactive constructed slots. These are extensions/reuse of initial owner maps, not another full initial snapshot. |
| Publish returned owner and results | Fixed result/handle assignment; no payload scan or new backing | Domain ownership moves into the returned owner; pointee/control-block lifetime can extend through retained aliases. No native publication occurs. |

Later operational staging/capture/command/matching limits are separate and are not
redefined by this inventory. Standalone unlimited helpers are not owner admission.
Initial recipe/projection preparation before `adopt` is outside this factory.

## Initial map prepayment: exact overlap and difference

For either input row type T, let n be its row count, t = sizeof(T), h =
sizeof(std::vector<T>), and K the sum of logical path/address payload bytes.
Every row has one canonical 36-character UUID, whose text charge is 37 including
the terminator. In [input_admission](../src/assembly/exchange_owner.cpp), its map
component is:

    S_map = h + n*(t + 37) + K

That represents one vector header, one fixed row representation, one UUID and one
nested payload copy. It does not reserve caller capacity slack in the destination.
There are no longer three by-value constructor parameters: the constructor takes
references and creates the final domain-backed projection/maps directly. Earlier
historical prose describing additional argument snapshots is superseded here.

For a fresh map constructed from the same rows with the optional meter enabled,
[ExchangeMap](../src/assembly/exchange_map.h) charges:

    M_map = 1 + n*(3*t + 74) + K

Derivation: one resize admission; n*t before outer reserve; n*(t+37) for default
slots/nil UUIDs; n*(t+37)+K for fixed-row assignment and `copy_key` (one comparison-
independent charge plus 36 UUID characters and nested payload). No outer relocation
is present in an initially empty map. Existing capacity, growth and shrink have
different paths and must not use this fresh-construction formula.

Thus M_map - S_map = 1 - h + n*(2*t + 37). The populated key payload K cancels.
The old prepayment already bounds a logical copy proportionally to input size;
it is not zero coverage. It also is not the full new map-operation accounting.
For n=0 the difference can be negative, so blindly adding an unsigned difference
would be wrong. S + M double-charges the initial representation. Applying the map
meter later without revising the input copy component would create that overlap.

Recommended next implementation: preserve the input byte census C+S, but replace
only its map **work** components with checked fresh-map construction demand,
retaining P (the logical projection-copy component) once. Prepay
P + M_occurrences + M_fields before any owner allocation. Keep map construction
under that prepayment rather than charging the full demand a second time in the
setup meter. Expose the breakdown and test it against actual metered map creation;
update existing tests that intentionally require `copy.charged == snapshot_bytes`.
This is a proposed change to work accounting, not a claim it is already built.

## Fixed objects and domain/control blocks

Selected allocation admission already covers owner/projection/coordinator/binding
objects and actual `allocate_shared` rebound requests for catalogue/declarations.
The latter contain their implementation-selected control blocks and padding;
adding guessed control-block sizes or `sizeof(pointee)` again would double count.
Weak aliases may keep a shared request alive after pointee destruction. The shared
allocator keeps its domain alive until that final request is released.

The reservation domain's own `make_shared` request is different: it is never
charged to itself. Existing independent lifetime evidence includes it; retained
census counts only `sizeof(ReservationDomain)`. #295's selected/independently
observed retained difference is 80 bytes on its toolchain, not a portable quota
constant. The focused layout probe reports a 64-byte domain object; the remainder
cannot be used as a universal control-block-size rule.

Initial setup charge 1 and membership collection charge 1 are fixed logical
admissions, not `sizeof`-based construction charges. There is no source basis to
claim all fixed initialization/control-block management is metered byte for byte.
Before declaring complete setup coverage, specify whether those admissions cover
fixed header/scalar initialization or add named fixed construction fees. Do not
silently reuse allocation-work bytes as algorithmic work. Stack, exception-runtime,
heap metadata/alignment outside allocator requests, other threads and driver/runtime
internals remain explicitly excluded from selected-request enforcement.

## Finite combined-admission design boundary

The next combined policy must state its accounting unit and ownership ledger first:

1. Keep caller C as conservative input capacity, not reconstructed allocator
   requests. Keep its overlapping snapshot S out of the new owner byte sum.
2. Obtain borrowed B from the trusted harness/resource owner, bound to the same
   execution/lifetime. `PreparedCapacity::numerical_bytes`,
   `PreparedStorage::workspace_layout/required_scratch/binding_bytes` describe
   particular layouts/reservations, not a complete host/device request ledger.
   Scratch includes component estimates; summing it with those components again
   duplicates backing. Do not accept an arbitrary request-supplied B as authority.
3. Maintain selected D dynamically at each allocation, including old/new overlap.
   Account the domain's own shared request H through a separately lived outer
   request tracker if it is to be inside the limit; do not guess H or make the
   domain own a control block that owns the domain.
4. An accounted-capacity policy may enforce checked C+B+H+D(t) <= L if C/B are
   explicitly defined, disjoint conservative charges. Call it **accounted bytes**,
   not total requested or physical memory. With fixed external baselines, its peak
   is C+B+H+max(D(t)); if any baseline changes, track simultaneous totals instead of
   adding independent maxima. An all-requested-byte policy needs actual external
   request records first; the present censuses cannot supply them.
5. Retain independent cumulative allocation work, logical copy/setup work and
   native capacity limits. Report the exhausted dimension and checked required
   amount; never convert one into another or refund cumulative work on release.

The inventory identifies the finite prerequisite: explicit work-component
replacement/fixed-fee policy plus a trusted disjoint borrowed-resource ledger and
domain bootstrap treatment. It does not authorize guessing missing coverage or
claiming complete GPU/driver/process accounting. Remaining operation integration
follows the bounded combined policy and its refusal controls.

## Evidence and required checks

Existing #295 witnesses are retained, not rerun: 47 scoped CPU passes; map setup
44 boundaries per row type; owner setup 7402/22443 boundaries including Bindings;
3272/5928 independent allocation failures reclaimed. `owner_input_admission` checks
zero-allocation input refusal, exact copy-work prepayment, capacity slack and
payload deltas. `shared_reservation_controls` covers rebound shared requests and
weak lifetime; owner lifetime controls compare selected versus observed allocations.

A new local, CPU-only arithmetic probe uses actual ExchangeMap constructors and
source `803437be`. It checks the formula above for n=0,1,3 for both row types with
two nested elements each, then requires zero selected live bytes after teardown.
It does not run a model or repeat the native baseline. Exact results and artifact
hash are recorded below after verification. Its first build command incorrectly
named a separate libcodec.a; codec objects already reside in libwarm_values.a.
The first failed command/log are retained. A second link attempt exposed the native
headers' LLVM ABI support reference; adding the installed LLVM 15 library resolved
that probe dependency. Both failures remain in the archive, not relabeled as passes.
Neither change modifies runtime code.

Implementation acceptance must add zero/one-short/exact work and byte limits,
checked sum/product overflow, each new refusal point before allocation, complete
unwind, unchanged input/native snapshots and retry. Use distinct/aliased resource
identities and overlapping growth/shared-weak lifetimes to prove no double count.
The existing owner and map witnesses should be extended, not replaced by a lower
setup-total target. No new runtime, hosted, GPU or complete-resource acceptance is
claimed by this documentation contribution.

Focused probe results (shared-host Clang 15, CPU; no native model execution):

| Row kind | n | S_map | M_map | Selected peak request bytes |
|---|---:|---:|---:|---:|
| Occurrence | 0 | 24 | 1 | 0 |
| Occurrence | 1 | 189 | 443 | 165 |
| Occurrence | 3 | 519 | 1327 | 495 |
| Field | 0 | 24 | 1 | 0 |
| Field | 1 | 187 | 441 | 163 |
| Field | 3 | 513 | 1321 | 489 |

Both row objects are 120 bytes on this toolchain. All six formula checks and
post-teardown zero-live checks passed. This deliberately small fixture demonstrates
counter meanings; it is not a full-owner peak or portable layout guarantee.
Verified local archive `setup-evidence/adoption-accounting-inventory-803437be.tar.gz`;
SHA256SUMS digest:

    e6f4c53bae9f1ae69e8b56deb90fc9a19b74fa45d10bb8271f29d60ce73c90ad

The archive contains source/binary/commands/results and both failed probe link
attempts. Raw machine-local artifacts have not been independently inspected
remotely. Documentation links and whitespace were checked; no accepted baseline
or hosted native run was duplicated for this source audit.

## Bootstrap tracker prerequisite — 2026-11-12 (awaiting review)

The new [shared bootstrap helper](../../common/README.md#domain-bootstrap-tracker-awaiting-review)
implements actual request tracking for its own outer storage and rebound domain
control block. Intrusive outer handles avoid introducing another untracked shared
control block. Shared allocators keep the tracker alive through weak references;
request work remains prepaid through allocator/constructor failure. It supplies a
separate H ledger; the selected domain remains D. This does not change owner
adoption yet, establish B, or enforce C+B+H+D. Owner integration and native refusal,
unchanged-state/retry controls are next, followed by the trusted disjoint borrowed
ledger and combined policy. No accepted native baseline is rerun for this isolated
helper; standalone compiler qualification is recorded in the admission evidence.

## Owner bootstrap integration (awaiting review)

Following #298 review 5879152987, `ExchangeOwner::adopt` accepts a separate
`ReservationBudget bootstrap_budget` and `ReservationResult bootstrap`. Input
inspection/copy prepayment and the initial setup gate still precede all allocation.
The tracker then admits its own request and the actual rebound domain/control-block
request before any selected owner allocation. Byte/work refusals return an empty
owner with bootstrap demand/status and zero live bytes; selected demand remains
untouched if bootstrap was refused. No hardcoded control-block size is used.

Successful owners retain a tracker handle; allocator-held handles also cover all
selected aliases and final weak releases. The result argument is an admission
snapshot, never a retained caller pointer. Later snapshots come from the retained
tracker. Existing selected-allocation and setup results remain independent.

Constructor setup/reservation refusal unwinds selected members and the domain,
then the last local outer handle. Bootstrap live bytes are reported as zero after
that cleanup; peak, prepaid work and refusal demand are preserved. Unexpected
exceptions, including bad_alloc at either bootstrap request or any later owner
request, follow the same cleanup and populate results before propagating. Failed
construction publishes no shared aliases. A failed outer allocation still reports
its prepaid work through create's local snapshot. Initial input/setup refusal
reports an untouched bootstrap result and performs no bootstrap request.

The new owner handle increases `sizeof(ExchangeOwner)` and its selected allocation
by one pointer on the qualified toolchain. The retained-capacity census still
includes the domain object, not the bootstrap tracker's allocation/control-block
requests; it is not a substitute for H or disjoint from it. Independent observation
checks H+D against current owner-related C++ requests in the qualified fixtures.
Do not add that census, input snapshot S or cumulative work to H+D. Borrowed native
backing, allocator internals and physical residency are outside these request
ledgers. Trusted B and combined C+B+H+D enforcement remain pending.

## Borrowed backing observability — 2026-11-12 (awaiting review)

[The native source audit](BORROWED-RESOURCES.md) follows #299 review 5879484896's
explicit instruction to identify missing native request surfaces rather than use
layout estimates. Common Device/LlvmDevice lacks a complete extent/live/incarnation
query; the executor allocation map misses direct prepared-launch requests and can
retain released Ndarray records. Tree roots, node/list pools and the owning Harness
components need explicit coverage. Array views are not allocations. The audit
records a passing focused CPU identity/alias/retirement probe and a finite native
hook/owner-export sequence. It does not supply B or enable C+B+H+D admission.
