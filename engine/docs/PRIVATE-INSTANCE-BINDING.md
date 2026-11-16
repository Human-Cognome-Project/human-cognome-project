# Private instance-binding contract proposal

Proposed under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343),
2026-11-16, awaiting review. Production source inspected:
`fa7f09e4fa0598ac74e0ba9fa93d050881e7d44f`, including integrated #349.
The source-accepted [#350 inspection](https://github.com/Human-Cognome-Project/human-cognome-project/blob/9e0307587dde72e793a4b661bc86f20ccb0cc654/engine/docs/NATIVE-INSTANCE-INSPECTION.md)
is an explicit pending dependency; its integer witness establishes bounded
native storage reuse, not an operational HCP instance-binding implementation.
This document specifies a private boundary; it adds no executable interface.

## Purpose and supported boundary

Bind a validated logical occurrence to the current native execution without
giving host metadata authority over numerical state, allocation or scheduling.
Share immutable source/semantic associations; retain separately staged mutable
claim mappings until their publication equivalence is proved. A binding is a
checked view under the exclusive execution owner, not another owner of cells.

The first implementation should use current V2/f64 prepared execution and its
actual committed RecordHandles. The indexed-storage requirements below define
what a later native layout adapter must prove; they do not promote raw SNode
coordinates into operational handles. Both forms obey the same identity,
lifetime, exposure and publication rules. No universal locator variant, new
public Analyst API, allocator, scheduler or hot-reload framework is proposed.

Runtime must outlive Harness; Harness must outlive the owner and borrowed
bindings. Calls and native kernels are exclusive at that owner boundary.
Metadata sharing neither extends a destroyed Runtime's lifetime nor permits
concurrent mutation. A detached observation is a separate value, not a lease on
current native storage.

Sharing is among authorized consumers of the same active owner. Private inputs
remain inside that boundary; neither a source pin nor a shared pointer grants
another owner or the shared cache permission to inspect them. Shared managers
continue rejecting private namespaces before lookup. The initial structural
subset still refuses local overlays.

## Data and authority

The following are required associations, not prescribed C++ class layouts.

| Association | Exact content / authority | Lifetime and sharing |
| --- | --- | --- |
| Source manifest | Owner and instance identity, study UUID, logical source generation, complete BundleRef (selection and revision), and exact ToolRefs (scope, address, revision) used by the composition | Immutable owner-held source pin. Current PreparedStudyOwner uses `shared_ptr<const ActiveAssembly>` plus the plan. Reuse those pins rather than consulting a mutable catalogue head during execution. |
| Occurrence identity | Existing `OccurrenceKey{study, OccurrenceId{entry,numeric path}}`; distinct repeated occurrences retain distinct entry/path even when ToolRef is shared | May reference validated immutable source tables by bounded ordinal. Text rendering is reporting work, not an identity comparator or an extra key registry. |
| Semantic association | Exact definition pin; ordered construction/contribution provenance; explicit StudyNode parent/ScaleRole and operative units; explicit typed FieldKey and exact participation counts where applicable | Shared immutable descriptor. Primary composition, sibling fields, membership/context and comparison topology remain separate typed relationships. SNode depth/index rank supplies none of these semantics. |
| Prepared-layout association | Native layout references and index mapping supplied by the preparing engine boundary; explicit rank/extents/field type, supported semantic role and compiled-layout lifetime | Borrowed in-process native references, validated before use. Retain pointer-free role/unit metadata for reports. Never export SNode pointers as portable identity. |
| Current claim association | For current execution, the complete native `RecordHandle{context, incarnation, address, record}` for each present body/field, aligned with immutable descriptor ordinals | Native claims are authoritative. The owner retains fixed-width copies of actual results. An absent/hidden occurrence has no invented future handle. |
| Publication identity | Owner/session/study/logical source generation plus current native context(s), native publication generation and completed tick | Logical source generation, native generation, record incarnation and tick have different meanings. A contact-changing tick may change native generation while retaining source identity and handles. |

Source/schema references: [projection identity](../src/assembly/letter_projection.h),
[warm values and pins](../../kernels/database/warm/values.h),
[admission associations](../src/assembly/structural_admission_plan.h),
[native record handles](../src/engine/record_map.h) and
[explicit layout/scale validation](../src/assembly/study_input.h).

The manifest must retain exact physical intrinsic bindings where needed, while
keeping physical mass/geometry distinct from source mass and exact contribution
denominators. It must not retain mutable position/velocity/force as a second
execution state. Current snapshots may contain legacy input/report fields;
sharing their source identity does not authorize numerical replay from them.

## Indexed native storage and stale-coordinate rejection

The #350 construction is one root, a pointer axis for two instances, two dense
nested axes for repeated subassemblies/parts, and typed place fields. Native
coordinates `(instance, subassembly, part)` identify cells of one compiled
layout. HCP occurrence paths map to those coordinates only through an explicit
validated association. An index is neither a world position nor a declaration
address, and `RecordHandle.record` must not be equated with it by convention.

For a future indexed adapter, a current binding requires all of:

1. The authorizing logical owner/handle matches before source/private payload
   access. The exclusive execution lifetime and expected native generation are
   current before following any borrowed layout pointer.
2. The native layout is still the prepared layout, with its owning Program/tree
   alive, expected shape/type and live backing where required. Tree/SNode IDs,
   pointer values and coordinates alone are insufficient: Program can reuse
   tree slots and destroy the previous host graph on replacement. A descriptor
   must be invalidated before that retirement/replacement; querying a dangling
   SNode to discover staleness is forbidden.
3. The occurrence's semantic/source descriptor and index mapping agree, the
   coordinate is in bounds, and the required native cell is active. Reading or
   navigating a hidden/absent part must not activate it.
4. A native lifetime guard rejects release/reuse at the same coordinate. Current
   prepared storage supplies full context/incarnation/address/record checks.
   A direct indexed path may instead rely on the existing engine publication
   generation **only if every release, reactivation and layout replacement that
   could create an ABA reuse invalidates that generation under the same exclusive
   authority before any binding is visible**. Otherwise it needs a native
   instance-incarnation guard; raw activation bits cannot supply that guard.

This deliberately does not require a host incarnation counter for every hidden
constituent. A guarded native instance cell plus validated relative paths may
cover its nested parts if that instance lifetime cannot change underneath a
current view. A part with an independently reusable live claim still requires
its own native claim guard. Guard exhaustion must refuse before mutation; no
wrapping or recapturing a new guard to make an old token appear current.

There is no demonstrated direct indexed HCP generation protocol in #350: its
ordinary native writes/deactivation bypass Harness publication. That witness
therefore fails the operational guard requirement by design. A later adapter
must establish it before replacing current RecordHandle validation. Native
activation, free lists and GC remain the storage authority; no second recycle
ledger is introduced. See [pointer operations](../taichi/taichi/runtime/llvm/runtime_module/node_pointer.h),
[NodeManager](../taichi/taichi/runtime/llvm/runtime_module/runtime.cpp),
[Program tree lifetime](../taichi/taichi/program/program.cpp) and
[borrowed lifetime limits](BORROWED-RESOURCES.md).

Example refusal: retain binding `(source occurrence A, generation g, coordinate
0)`; retire A and publish the native change; reactivate coordinate 0 for B.
Resolving A must fail its generation/incarnation check before reading B, even
if its SNode IDs, coordinate and numerical values happen to match. Refreshing
the guard on A without revalidating its logical association is forbidden.

## Current owners and the proposed shared boundary

| Concern | PreparedStudyOwner on inspected main | ExchangeOwner on inspected main | Contract / first candidate change |
| --- | --- | --- | --- |
| Source and semantics | Immutable ActiveAssembly pin, StructuralAdmissionPlan, pointer-free reporting scales | Committed/spare LetterProjection, immutable declarations and MembershipFields | Share validated source/role associations by const ownership or bounded references. Mutable exposure stays in committed/candidate state. Do not replace the spare projection with a shared mutable object. |
| Initial claims | Empty prepared Harness; reserve metadata, native combined bootstrap, resolve actual command results | `adopt` validates caller-supplied trusted committed claims and constructs fallible metadata | Current bootstrap is the source of initial claim authority. Calling allocating `adopt` after native commit remains an unsuitable publication step. This proposal does not unify the two owners or extend their operation sets. |
| Map shape | Plan-key ordinals plus fixed-size `bodies_`/`fields_` handle arrays | Owner, ExchangeBindings and coordinator each retain occurrence/field map storage | Candidate for sharing is immutable key/pin/role storage. Committed, captured and prospective handle/participation state have different transaction lifetimes and cannot be collapsed merely because they contain similar rows. |
| Currentness | `ready` checks Harness generation, live inventory, map contexts and every full header; `tick` follows native generation | Capture validates full headers/source participation; execute validates expected generation and native preflight | Preserve all these checks. A generation change does not grant permission to relabel stale claims. Existing owners do not yet exchange one common binding object. |
| Publication | Prepaid handle assignments, verification, then Published phase; postclaim faults make execution unready | Coordinator resolves actual handles/swaps maps after commit; outer owner swaps maps/projection and advances generation | One exclusive visibility boundary after native commit. Keep sufficient candidate storage for refusal/retry and postclaim failure behavior. No allocating key copy, discovery callback or ordinary resource refusal may remain after commit. |
| Observation and teardown | Adapter builds detached reporting mapping; owner teardown releases metadata but does not retire borrowed state | Reporting uses retained projection/maps; teardown releases its owned metadata, not native claims | Observers consume the same published associations and native completed state. Borrowed-view lifetime ends at the next mutation/retirement; detached reports retain their original identity/tick and never become current by aliasing. |

Exact sources: [PreparedStudyOwner](../src/assembly/prepared_study_owner.cpp),
[ExchangeOwner](../src/assembly/exchange_owner.cpp),
[capture/validation](../src/assembly/exchange_bindings.cpp),
[coordinator commit](../src/assembly/exchange_coordinator.cpp),
[owner resource inventory](OWNER-RESOURCES.md) and
[prepared observer](../../tools/viewport/native/prepared_owner_state.cpp).

Fields retain the complete semantic key `(GroupKind, ObjectKey)` even when a
particular container uses an address/object lookup shortcut. Current exchange
map helpers select `field.object` as their storage key, while capture checks
typed participation. A shared descriptor must preserve kind validation and
supported collision/refusal behavior; using an untyped object key is not a new
definition of field identity. No broader field namespace policy is chosen here.

## Publication and failure protocol

Preparation pins/validates the immutable associations and reserves candidate
handle/state buffers under existing work/reservation limits. A prepared
descriptor is not a current native binding. Capture and sealing name the exact
expected native generation and trusted survivor handles. Future handles are
sealed command references, never guessed record slots.

Immediately before commit, revalidate authorization, lifetime, generation,
complete source/role associations and native claims. Reuse the current native
placement/source/comparison/focus/contact transaction. After success, resolve
actual results into already reserved candidate storage, verify full handles,
then publish the descriptor/claim/exposure/generation association together.
Copy command results before placement scratch can be reused. Readers cannot
observe an old source descriptor paired with newly committed claims.

Ordinary precommit refusal leaves the healthy committed publication unchanged
and supports the operation's existing retry rules. Candidate metadata may be
overwritten on retry; it is never a completed observation. A postclaim backend
or invariant failure leaves the execution unready and partial bindings
inaccessible; no healthy rollback or generic reset is promised. Preserve the
separate native result/status and logical authorization/refusal behavior rather
than defining a new universal status enum.

The proposal uses existing native publication generation to invalidate borrowed
views. Unaffected instance state/source pins must survive a targeted operation;
its old generation-bound view may still require reacquisition. That is a view
refresh, not rebuilding the unrelated instance or restarting the study. The
next implementation must prepay that bounded lookup/validation and must not
silently stamp an old observation with the new generation.

## Exposure, catalogue and unsupported requests

Recipe construction and source pins remain available when an aggregate or its
parts are hidden. Exposure chooses physical participants; it does not manufacture
extra mass or claims to support navigation. On an accepted D7 exchange, preserve
source occurrence identity and unaffected claims while replacing outgoing
physical claims through the existing transaction. Re-exposure must resolve new
actual claims; an old incarnation must not become current again. Preserve D7
placement at the outgoing centre or nearest unoccupied position and its
conservation rules; the binding does not select a new rollup/force law.

Publishing an unused catalogue item does not mutate this manifest or invalidate
its execution. A pinned definition changes only through an explicit supported
adoption operation; no background head lookup or automatic migration occurs.
The current Bundle validator/projection permit one revision per ObjectKey.
Requesting two revisions of that identity within one study must remain refused
as unsupported by this contract's first implementation, with healthy committed
state preserved. Do not invent addresses or rewrite existing pins to emulate it.

Mixed-revision provenance, heterogeneous/ragged compiled layouts, arbitrary
contextual attachments, local-overlay structural admission, new field/schema
growth, public producer migration, general live edits and complete accounting
remain separately reviewed work. Membership never silently becomes composition.
Explicit comparison roles/contributions remain inputs; mathematical coherence
frames and degrees of freedom under #324 are not implemented by this binding.

## Acceptance required before changing representation

| Control | Required evidence |
| --- | --- |
| Exact immutable association | Reject wrong owner/session/study/source generation, wrong BundleRef/ToolRef, role/unit mismatch, duplicate numeric paths, missing/aliased claims and incompatible field kinds before native mutation. Sharing const storage must preserve pin lifetime after caller temporaries die. |
| Repeated nested instances | Two occurrences of one definition, including repeated subassemblies, select distinct native cells/claims under an explicit path mapping. Changing one preserves the other's source, intrinsic/numerical state, membership and claims. |
| ABA/lifetime refusal | Retain old bindings across native release/reuse, tree replacement and Runtime/context replacement. Reject them before accessing new payload. Exercise a reused coordinate with equal values to rule out value-based validation. Never dereference a dead layout to perform the check. |
| Generation and observations | Tick with contact generation change preserves full current claims. Old borrowed views and copied observations retain old identity; freshly acquired views/observations report the new native generation/tick/frame. No view extends Runtime lifetime. |
| Transaction publication | Precommit work/capacity/invalid-input refusal preserves committed source, claims, exposure and numerical state; retry succeeds. Inject postclaim/prepublication failure and prove unready status with no readable half-publication. Reusing scratch cannot change committed handles. |
| Exposure | Aggregate/parts alternatives retain logical identity; only exposed participants claim physical state. D7 conservation/placement and survivor state remain governed by existing exchange controls. Navigation/readback activates no hidden parts. |
| Catalogue isolation | Existing pins and unrelated active state survive unused publication. Reject mixed revisions of one ObjectKey and incompatible layout requests explicitly; no hidden rebuild or fallback policy. Source inspection alone does not qualify a future publication/adoption implementation. |
| Resource/compilation equivalence | Account once for shared immutable pointees and separately for each live/candidate buffer; include sharing/control-block setup and borrow lifetimes. Verify allocation/work refusal and first-use/reuse boundaries with native backing/compilation evidence. Capacity charges, allocation requests, reserved pool extents and RSS stay separate; full B remains incomplete. |

Existing [owner tests](../tests/prepared_study_owner_test.cpp),
[exchange tests](../tests/admission/README.md) and
[observer tests](../../tools/viewport/native/prepared_owner_state_test.cpp)
are comparison evidence, not proof that this future shared binding is built.
The native #350 witness must gain an authoritative guard/publication protocol
before it can qualify the direct indexed case. Use the required worker/path
matrix for whichever execution boundary changes; do not rerun an accepted
baseline solely to review this documentation proposal.

## Smallest implementation proposed for review

First introduce a private immutable association view over the existing pinned
PreparedStudyOwner plan/source, with checked ordinal access and explicit
source/role validation, while leaving actual handle arrays, candidate state and
both owners' operation sets intact. Prefer existing types/accessors where they
meet the contract. Integrate one existing observer/validator consumer and prove
exact association, refusal and lifetime equivalence. Do not retain supposedly
immutable spans into ExchangeOwner's mutable exposure/maps; a future shared
recipe/descriptor pin must own stable storage, or a borrowed view must expire
before mutation. Measure eliminated key/pin copies and retained sharing overhead
before removing a proven duplicate store.

A later, separate native indexed adapter must prove the lifetime guard and
transaction requirements above. Its representation can then be compared against
the current handle path using the same two-instance controls. This sequence
does not require retaining a parallel flat ledger for every hidden constituent,
and does not claim the current flat solver has already been migrated.

## Validation and handoff

This slice changes documentation only. Source/symbol checks, local-link checks,
repository boundary checks and diff hygiene qualify that scope. No native
baseline rerun or database mutation is needed. Record the public source, actual
check results and pending hosted disposition with the contribution; GitHub CI
remains authoritative. No implementation acceptance follows from merging this
proposal.
