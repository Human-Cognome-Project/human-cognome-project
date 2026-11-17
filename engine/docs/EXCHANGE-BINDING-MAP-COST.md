# Retained source-binding maps and publication transport

Contributor inspection under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343),
2026-11-17, awaiting review. Public tested source:
`120658ca6c713b5fd5e0e75bb8aec015ee9954db`.

Starts from main `068fe63a`, which integrates #363. Normal merge `d2e599c1`
preserves source-accepted, unmerged #364 `45d30de3` and both current handoffs.
Production is byte-identical to #364. This slice adds a test witness and bounded
proposal; it does not remove maps, introduce runtime borrowing or claim savings.

## Required association and current owners

[`ExchangeOwner`](../src/assembly/exchange_owner.cpp) owns the complete current
occurrence and field maps from trusted admission results. Occurrence rows pair
`OccurrenceKey{study, entry, numeric path}` with the full native
`RecordHandle{context, incarnation, address, record}`. Field rows pair the typed
`FieldKey{GroupKind, ObjectKey{scope, address}}` with the full field handle.
Repeated/equal-valued alternatives and distinct field kinds remain distinct.
The recipe, declarations and catalogue retain the complementary complete source
pins/provenance; these maps do not replace that meaning.

The private [`ExchangeBindings`](../src/assembly/exchange_bindings.cpp) retains
another pair. Adoption constructs their full capacity while their active lengths
are zero. Every preparation clears their active lengths and copies all current
owner rows in order. Capture reads that copy to match projected active identities,
full current headers, exact source masses and declared outgoing participation.
It also retains independent masses, outgoing handles, ordered captured source
edges and used-row masks. Those captured facts have their own revalidation role.

[`ExchangeCoordinator`](../src/assembly/exchange_coordinator.cpp) owns a third
pair for prospective associations. Command preparation copies surviving current
occurrences and the trusted field inventory, appends incoming keys with pending
claims, then removes retired fields. Candidate storage remains necessary while
ordinary refusal/retry preserves the healthy current owner.

The existing [binding contract](PRIVATE-INSTANCE-BINDING.md) distinguishes these
transaction lifetimes. Similar row shapes alone do not authorize collapsing them.
The proposed change below retains the current/candidate distinction and all
captured validation facts; its target is the binding's intermediate map copy.

## Actual publication lifetime

Source tracing gives these intermediate roles, with A denoting complete current
associations and C denoting prospective associations. Executable checks observe
the states before and after the exclusive operation, without interrupting commit:

| Point | Owner pair | Binding pair | Coordinator candidate pair |
| --- | --- | --- | --- |
| After adoption | A | Empty, reserved | Empty, reserved |
| After successful preparation | A | Copy of A | C, including pending command claims |
| After coordinator native success/claim resolution and map swaps | A | C with actual claims | Previous binding A |
| After outer owner map/projection/report/generation publication | C with actual claims | Previous owner A | Previous binding A |

Both swaps occur within one exclusive native/owner operation. There is no healthy
externally observable intermediate publication. Precommit refusal leaves A usable;
postclaim/prepublication faults leave the Harness unready. Old captured authority
is stale after publication and cannot be relabeled with the new generation.

The new test snapshots full previous-source keys/handles independently outside
execution/allocation guards. Before execution it compares owner and binding
active rows and records all six outer backing addresses. After each successful
exchange it verifies the exact three-way rotation, both historical pairs against
the detached previous-source values, the new owner maps against actual native
headers, coherent native/owner generation and stale old binding validation.
Original single/mixed/equal owner guards, retries, one-short limits, failures and
following ticks remain. Probe snapshots are test scratch, not retained production
memory or extra numerical authority.

## Native overlap and limits

[`PreparedRecordMap`](../src/engine/record_map.h) binds real native presence,
address/incarnation/declaration fields to its bounded record pool; its
[implementation](../src/engine/record_map.cpp) builds those fields/kernels on
Taichi SNodes. [`PreparedStorage`](../src/field/prepared_storage.h) retains actual
claim headers and committed/prospective exact source masses, relationship
endpoints/ordinals/shares and counts. Native state, claims, generation, placement
and publication remain authoritative.

Those structures do not encode the HCP study/entry/path, typed field scope/
address or complete recipe provenance. Native record numbers, coordinate reuse,
equal mass or matching outer pins cannot recover or authenticate that association.
The [native instance inspection](NATIVE-INSTANCE-INSPECTION.md) and
[indexed guard contract](PRIVATE-INSTANCE-BINDING.md#indexed-native-storage-and-stale-coordinate-rejection)
remain applicable. This inspection proposes no direct indexed replacement or new
native lifetime protocol. It reads existing semantic associations from their
current exclusive owner instead of proposing that native cells supply them.

## Independent request attribution

`--case exchange_binding_map_cost` runs the one-selection, four mixed-exposure
and three equal-alternative native fixtures through actual owner adoption and
binding stage/capture. Expected values, caller copies and visitor scratch are
prepared before the current-thread C++ allocation window. The actual caller
projection and input-vector copies then die. All original owner cases also run
the witness before their unchanged controls; a dedicated recipe-only run retains
its former scope.

The visitor attributes outer slot backing, every constructed UUID string and
path/address buffer to exact independently observed requests and the selected
domain. This includes inactive slots without treating them as live claims. It
rejects within/across-family aliases and agrees with the existing six-map census.
After staging it compares complete ordered logical/native associations, verifies
all buffer addresses/capacities unchanged, captures the real source successfully
without allocation, and confirms candidate active lengths remain empty in that
probe. Comparator controls alter each handle component, study, entry/path, field
kind, scope and address independently. Teardown releases all selected backing,
then the final weak-domain/bootstrap allocations, without changing native state.

On Linux x64, Clang 15/GCC 11 agree:

| Fixture | Whole owner retained / scoped peak | Binding map requested bytes / blocks | Stage logical work |
| --- | --- | --- | --- |
| One accented-letter selection | 289,584 / 289,584 | 47,180 / 562 | 674 |
| Four copies, mixed exposure | 341,816 / 341,816 | 47,180 / 562 | 2,523 |
| Three one-byte copies, mixed exposure | 305,194 / 305,194 | 47,180 / 562 | 1,512 |

Each fixture reserves 140 occurrence slots and 140 field slots **per pair**.
All three pairs have the same 47,180-byte request footprint (141,540 total map
backing bytes). Actual active lengths differ; the fixed capacities explain the
same retained candidate cost across these fixtures. These are not per-live-body
costs or a general production estimate. No removal is implemented.

The binding's 112 bytes of map headers are embedded in its actual 376-byte object
request. They are not another allocation and cannot be added to the 47,180-byte
candidate cost. The three map reports are already part of whole-owner totals.
Census capacity, requested bytes, logical work, cumulative allocation work and
scoped peak retain their separate meanings. Allocator metadata, RSS, whole-process
peak, native pool capacity and timing are outside these measurements.

Staging allocates no C++ memory but copies current rows. Its measured fee includes
one fixed direction/association unit plus row/key copy work. A future removal may
omit only eliminated copies/reservations; it must retain association validation
and the independent capture/command/execute work. It cannot simply delete the
entire stage fee or use empty owned binding maps to underprice source validation.

## Bounded proposal for review

After review, give only the private persistent owner path synchronous const access
to its existing current map pair during stage/capture, command construction and
immediate precommit validation. Prefer explicit private call arguments to stored
spans or a new wrapper/registry. Such access must not escape the exclusive call,
remain live across owner-map mutation, or refresh a stale captured generation.
The standalone value-taking binding/coordinator adapters retain independently
owned map behavior and caller-source lifetime.

Keep the coordinator's independently reserved prospective pair and all captured
masses/outgoing handles/edges/masks. On native success resolve actual claims into
that existing candidate pair, then publish it directly into the owner together
with projection/reports/generation. The source loan must end before that map
mutation. This removes the middle map transport, not candidate storage, source
revalidation or the sole native transaction. Preserve existing postclaim failure
semantics, including the owner-publication failure point.

Coordinate the private owner/binding/coordinator paths, both publication modes,
census and independent visitors. Keep initial input prepayment at one retained
owner map snapshot; remove only actual eliminated backing/copy/reservation work.
Adoption's temporary value-taking validation bindings are a separate measured
surface: do not infer their elimination from this retained pair's cost. Do not
subtract embedded headers without measuring the final object extent. Measure
actual whole-owner before/after requests, control/argument overhead, setup
transients/peak, selected-domain and final alias/weak cleanup.

Before implementation acceptance, run the existing complete native owner/exchange
worker/compiler/path matrix, preserving full typed keys/pins and all four handle
components, stale/foreign/wrong-role/generation refusal, complete source mass and
ledger/claim validation, same-count/equal-value alternatives, both directions and
survivors, ordinary retry and postclaim unready behavior, following ticks, every
allocation failure and exact/one-short work/bytes. Scope any field-collision policy,
public producer migration or native indexed guard work separately. This proposal
does not authorize collapsing captured participation facts or borrowing a mutable
map as an immutable cross-publication descriptor.

## Local qualification and handoff

Native CPU/x64, C++17 Release, Clang/LLVM 15 and GCC 11, matched repository
fork/runtime, GPU backends/qualification disabled. Incremental builds recompile
the exact recorded source; the GCC cache was freshly configured/qualified for
the preceding #364 work and explicitly reconfigured here. Independent owner
scenarios run at most two concurrently per compiler on the shared host.

| Local scope on the exact clean tested source | Result |
| --- | --- |
| Quick full native build/backend, three CTests, 632 placement arithmetic samples and placement capacity | Passed |
| Dedicated three-fixture probe, Clang/GCC × 1-worker bulk / 2-worker direct | Four runs passed, 12 fixture measurements |
| Full single/mixed/equal owner cases, Clang 1-bulk and GCC 2-direct | All six passed |
| Added before/after publication controls | 32/32/16 per compiler, 160 total, with original following ticks/guards |
| Production-source comparison against reviewed #364 | Identical; no runtime map removal or borrower |

Default owner allocation-failure, exact/one-short work/bytes, source/owner/shared/
weak/domain teardown, wrong/stale/foreign/directional refusal, ordinary retry and
postclaim failure controls remain enabled. Observer scratch/snapshots are outside
measured owner windows and execution-allocation guards. Source/HEAD are clean
and unchanged before/after each profile; manifests and binary identities verify.
No full regression/compiler matrix, database, GPU, timing or complete owner budget
rerun is claimed. Existing post-PASS cache-lock warnings are retained; no cache
was cleaned. Authoritative hosted CI qualifies its own full scopes.

Evidence is contributor-local, not remotely available or independently executed
maintainer evidence. GitHub CI is authoritative for its gates. #364 source review
5945881346 permits continuation; its hosted/integration disposition remains
separate. Do not duplicate its running workflow.

At handoff #364 smoke/warm/standalone values and native build, database, all four
compiler jobs and regression shard 1 pass. Native 44166963726 still has regression
shards 2/3/4 running; status records are retained and no rerun was dispatched.

Retained evidence: `/tmp/hcp-binding-map-inspection-120658ca-evidence.tar.gz`.
SHA-256:

```text
2193b78b52171bdf5c8c3c244541ff6f3fc2f0ce9b5f981ba5fede2f3b3e5ac9
```

It contains all three final profiles and verified manifests, exact commands,
source/submodule/build/toolchain/binary identities, retained Clang/GCC native
executables for a future actual before comparison, full probe/publication logs,
development diagnostics and dependency status records. The archive listing was
checked after creation. Later handoff changes are Markdown only relative to
the tested production/test source.

Review this inspection/proposal before implementing the private owner source-map
path. Catalogue/mixed revisions, direct indexed guards, broader producer adoption,
full combined accounting and #324 mathematics remain separate. Held #75/#111/#123
are unchanged; no new Analyst/control surface or execution authority is added.
