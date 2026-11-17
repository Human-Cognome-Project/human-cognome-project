# Membership field key ownership and cost

Contributor comparison under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343),
2026-11-16, awaiting review. Public tested source:
`ec21b153be936b579676027179358ed9f6b34418`.

Follows the next-comparison direction in
[#356 review 5930271128](https://github.com/Human-Cognome-Project/human-cognome-project/pull/356#pullrequestreview-5930271128).
Starts from main `0bd4db55`, which integrates #354, with source-accepted #356
`c9c414a2` / #355 `93dcd711` explicitly preserved in dependency merge `2697fb36`.
Those dependencies remain unmerged. Production is identical to #356; this
comparison changes only the membership value test and Markdown. It implements
none of the removal proposal below.

## One duplicate immutable association

`MembershipContribution` owns an occurrence key, a typed FieldKey, a complete
target ToolRef and an exact count. The collector always emits membership kind
and sets `field.object` equal to `target.object`, copying both objects into
independently owned buffers. The target pin already supplies the complete
scope/address and revision. This comparison selects only that extra field
ObjectKey; occurrence identity, field kind, full target revision and count
remain required.

The [accepted membership contract](MEMBERSHIP-FIELDS.md) follows actual pinned
upward memberships, deduplicates targets per owning letter and emits them per
active occurrence. Bytes inherit their owning letter's groups. Repeated uses
and language contexts stay distinct, even when they share the same byte pin.
NULL group naming mass is metadata and creates no replacement particle.

The retained alternative catalogue uses the same collector with all exposure
alternatives, including hidden rows. ExchangeOwner shares that immutable
catalogue with bindings while maintaining separate current/candidate state.
Its keys remain owned after projection/fixture teardown. A reference to the
already owned target object can live with the same row/catalogue; it need not
borrow a source recipe or extend native lifetime.

Sources: [row and collector interface](../src/assembly/membership_fields.h),
[actual collector and bind readers](../src/assembly/membership_fields.cpp),
[prepared-plan validation](../src/assembly/structural_admission_plan.cpp),
[exchange reader](../src/assembly/exchange_bindings.cpp),
[existing retained census](../src/assembly/owner_storage.cpp),
[owner resource lifetime](OWNER-RESOURCES.md).

Native record handles identify current execution claims; they do not carry
this full logical group pin or replace its revision. The existing `target.object`
member supplies the candidate reuse boundary without another registry or owner.

## Actual producer witness

The [existing membership test](../tests/membership_fields_test.cpp) now measures
the actual public live factory and selected-domain live/catalogue collector.
Fixtures contain repeated English uses, a French use of the same byte,
aggregate/parts exposure, a membership diamond and NULL naming anchors.
Independent expected keys, pins and whole counts are prepared before the
measurement window from the fixture's known English/French group sets.

The fixed-table allocation observer attributes field/pin UUID and address
buffers by their actual live pointers. It verifies equal keys with distinct
backing, full target revisions, numeric entry/path, field kind, exact count and
selected-domain ownership. Group addresses have two, three and five elements;
these are valid group identities, not a reason to impose the physical-origin
five-element restriction.

Comparing the existing pin object with the field object allocates no C++
storage. The borrowed collector projection and fixture then die, while the
returned rows preserve exact keys/pins/counts and backing. Independent recipe
and expected-value copies remain outside the window for assertions; they are
not the projection borrowed by the collector. Tracked buffers acquired during
the producer establish ownership independently of those copies. Destroying the
result releases every observed request and returns selected-domain live bytes
to zero. No Runtime, native admission, database or cache lookup is created.

## Measurements

Clang 15 and GCC 11 observe the following on this Linux x64 ABI:

| Actual producer | Public live | Bounded live | Bounded alternative catalogue |
| --- | --- | --- | --- |
| Recipe version | 1 | 2 | 2 |
| Rows / vector capacity | 8 / 8 | 8 / 8 | 16 / 16 |
| C++ allocation requests | 225 | 225 | 277 |
| Cumulative requested bytes | 16,707 | 16,707 | 23,459 |
| Retained requested bytes | 4,191 | 4,191 | 8,255 |
| Scoped peak requested bytes | 14,235 | 14,235 | 18,299 |
| Duplicate field-object nested requests | 16 | 16 | 32 |
| Duplicate field-object nested requested bytes | 380 | 380 | 760 |
| Same object backing inside full pins | 380 | 380 | 760 |
| Enclosing contribution-vector requested bytes | 2,688 | 2,688 | 5,376 |
| Pin-object comparison allocations | 0 | 0 | 0 |
| Tracked live bytes after destruction | 0 | 0 | 0 |

An ObjectKey occupies 88 bytes, FieldKey 96 and the current row 336. The selected
88-byte member extent occupies 704/1,408 bytes inside the live/catalogue vector
requests; it is not a separate allocation. The measured 380/760 bytes are its
additional nested storage. Retaining the explicit kind means the whole 96-byte
FieldKey is not claimed as removable. These are currently occupied candidate
costs, not realized savings: actual row padding, capacity, relocation, producer
peak and copy-work changes need an implementation measurement.

The live/alternative counts differ deliberately; do not drop hidden alternatives
or merge repeated occurrences to obtain a saving. Scratch maps/sets/frame
storage have distinct traversal lifetimes and are not selected for removal.
Counters cover current-thread new/new[] requests and releases in the collector,
not arbitrary malloc, RSS, allocator rounding, native/device storage, process
peak or latency. Selected-domain retained bytes equal tracked retention in
these bounded fixtures. Domain construction, the stack result object and
ExchangeOwner's shared-pointer/control-block construction are outside the
window. This does not complete the owner resource census or full B accounting.

## Bounded removal proposal — pending review

Use the existing complete `target.object` as the membership field's object
authority. Retain an explicit GroupKind with membership semantics, full ToolRef,
complete occurrence identity and exact count. Preserve `(kind, object)` field
distinction from sibling-definition and parent-value fields, even when their
ObjectKeys agree. No new registry, canonicalization table or owning wrapper is
needed; do not return a freshly owning FieldKey merely to compare or render it.

Update the collector, legacy/structural bind readers, ExchangeBindings,
StructuralAdmissionPlan, retained census and all pure/native tests together.
Use existing kind/object arguments and the full pin object at those boundaries.
The current plan validator explicitly checks field-object/pin agreement and
validates the target against the pinned recipe. Preserve complete pin/count/
role checks and reject any inconsistent legacy duplicate if a transition still
accepts one; do not introduce a legacy fallback solely for this cleanup.

Preserve shorter valid group addresses, exact scope/revision, memoized-visit pin
validation, missing/cyclic-source refusal, direct/diamond per-occurrence dedup,
English/French context isolation, repeated/multibyte source counts and NULL
naming anchors. Catalogue/live distinction, study mismatch refusal, all exposure
combinations and detached/shared-catalogue lifetime must remain. Full native
claims, candidate/committed projections and publication state are separate
required ownership; this proposal removes none of them.

Adjust legitimate construction/relocation/census charges to actual storage;
preserve traversal and source/association validation work. Retain exact/one-short
work/allocation controls, GCC unwind discipline and allocation-failure coverage.
Measure the actual changed public/bounded/catalogue producers before/after,
including retained bytes, peak and kind/accessor overhead. Reuse native membership
and owner/exchange/observer controls for implementation. A scoped removal needs
review of this boundary; it does not migrate public producers or adopt a new
catalogue revision policy.

## Qualification

Personally executed at public `ec21b153`, matched CPU-only LLVM 15 fork with
the exact recursive submodules recorded in the evidence:

- `bash engine/scripts/run_local_ci.sh quick /tmp/hcp-membership-ec21b153-quick`:
  PASS. Full native rebuild, actual CPU, three core CTests, 632 independent
  placement interval samples and placement capacity.
- `flock engine/build/.local-ci.lock bash /tmp/hcp-membership-ec21b153-checks.sh focused`:
  membership value/native CTests both passed in 9.20 seconds. The value test
  passes all 3,808 checks, including the three actual collector witnesses and
  existing context, exposure, revision/cycle, deep traversal, exact/one-short
  work/allocation and teardown controls. The native consumer retains its
  inclusive-centroid/mass/field-kind/participation checks.
- `bash /tmp/hcp-membership-ec21b153-checks.sh clang` and `... gcc`: fresh
  standalone builds each passed all four projection/membership/structural-recipe/
  bootstrap reservation CTests in 0.18/0.17 seconds. Every actual collector
  measurement agrees across native Clang and standalone Clang/GCC, without
  Taichi/PostgreSQL linkage in the standalone builds.

Development first hit an ambiguous fixture name, then a structural fixture
missing explicit focus. A local overbroad focus correction was refused by the
legacy v1 contract. The corrected source supplies focus only in structural
modes, without changing production or a source contract. The failed logs and
initial witness remain preserved; public development `628f16c5` is followed by
the normal corrective commit `ec21b153`. They are not relabelled CI passes.

Formal runs required the same clean corrected source and preserved source-after
checks, commands, configurations, submodule pins, logs and binary hashes.
Per-file manifests verified. Final notes change only five Markdown files after
the tested source. Raw directories:
`/tmp/hcp-membership-ec21b153-{quick,focused,clang,gcc}` and
`/tmp/hcp-membership-cost-development`. Archive
`/tmp/hcp-membership-ec21b153.tar.gz` excludes standalone build products;
SHA256 `1a4ef6ed19610f9b6583075281968bdb04895ad783fa761a58ffe2f753a4d72f`.
This remains contributor-local execution, not independently inspected remote
raw evidence. GitHub CI is authoritative. Full regression/compiler/database
and owner/exchange/observer matrices, browser, GPU and native Windows were not
rerun locally for this test-only comparison.

#355 native run 43978139830 now reports success; its integration remains
separate. Source-accepted #356 still has its own pending hosted qualification.
Review this candidate and its scoped removal proposal before implementation;
no duplicate workflow dispatch is requested. Direct indexed guards,
catalogue/mixed revisions, public producer migration and #324 mathematics
remain separately scoped.
