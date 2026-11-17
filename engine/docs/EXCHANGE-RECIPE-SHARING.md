# One immutable recipe for an exchange owner's two views

Contributor implementation under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343),
2026-11-17, awaiting review. Public tested source:
`6be856c78b2401d997493ad8ea37a46420e13df1`.

Implements [#363 review 5944572189](https://github.com/Human-Cognome-Project/human-cognome-project/pull/363#pullrequestreview-5944572189)
following the [actual duplicate-recipe inspection](EXCHANGE-RECIPE-COPY-COST.md).
Starts from main `c63a1c24`, which integrates #362. Normal merge `7550839e`
preserves reviewed, unmerged #363 `553834c6` and both current handoffs. The
implementation's tree is `c845e7e7164791b4212c4a4041cc62de64de3be5`.

## Ownership and value operations

Each [`LetterProjection`](../src/assembly/letter_projection.h) retains a
`shared_ptr<const warm::Bundle>` in place of its embedded Bundle. Initial owner
adoption deeply clones the caller's complete recipe into the selected domain.
Only [`ExchangeOwner`](../src/assembly/exchange_owner.cpp)'s explicit private
`OwnerSpare` constructor path retains that exact recipe for the second view.
It checks the selected domain as a necessary association constraint; equal
domains and matching outer pins do not authorize sharing in value-copy APIs.
Different owners still adopt different recipes.

The complete Bundle remains semantic authority: selection/revision and owner/
creator pins, tool revisions/epoch/author/recipe/notation/mass, ordered parent
targets/ordinals/stored masses/counts, memberships, geometry, conceptual nodes,
occurrences, initial selections, contacts and focal fields. Const recipe access
remains allocation-free. Live exposure, occurrence activity, both parent pools,
legacy/structural input state, maps, claims, generation, catalogue and atomic
publication remain independently retained by their existing owners. Publication
still swaps whole projection owners; it never mutates the recipe.

Ordinary copies detach all selected allocators and deeply copy the recipe.
Explicit domain copies deeply copy it even into the source's domain. Copy
assignment stages a complete independent value in the destination's domain
before replacement; failure preserves its previous state and live-byte charge.
Moves transfer backing and domain without allocation. A moved-from projection
is assignable/destructible; recipe access rejects its missing recipe. Self-copy
and self-move preserve the value.

Qualification exposed an equal-domain libstdc++ string-move accounting problem:
memberwise move assignment could leave displaced UUID backing in an allocator
whose domain had moved. Projection replacement now destroys the destination
and invokes its noexcept move constructor in the same storage. This preserves
domain accounting without changing the shared allocator for other components.
GCC 11 also exposed a partial aggregate-initialization leak in public projection
creation. Allocating occurrence identities, provenance, input text, parents and
contact members are now staged as named values before moving them into records.
The same staging repair covers v1 warm validation's temporary occurrence
conversion. Their values, validation rules and public/native meaning are unchanged.

## Admission and census

The initial [input gate](ADOPTION-ACCOUNTING.md) still inspects/prices one complete
projection snapshot and the two initial maps. Its projection object plus shared
Bundle payload header are included once; this adds the pointer representation
to that conservative header charge. Actual payload/control-block requests and
all nested backing use the selected allocator. Initial admission does not price
a second recipe or a second complete projection snapshot.

Spare inspection/copy charges only its new mutable view. It omits the eliminated
recipe traversal, logical copy and nested allocation work, while charging one
explicit fixed recipe/domain association. Full pin, source, current exposure,
membership, claim, binding and publication validation remain in their existing
paths. Both active/spare parent reservations remain metered. Exact/one-short and
every-boundary byte/work retries use recalculated observed demand.

In the single-selection fixture, initial projection copy work is 32,362 → 32,378;
caller/snapshot bytes are 34,107/33,083 → 34,123/33,099. The combined input gate
therefore rises 67,190 → 67,222, with inspection work unchanged at 1,905 and map
components unchanged. This prices the new pointer representation once per input
view/snapshot. Separate setup logical work falls 574,972 → 547,935; binding
boundary coverage remains 3,813. These work/capacity figures are not additional
requested-byte savings or a combined budget.

[`projection_storage()`](../src/assembly/projection_storage.cpp) counts both
projection objects, their independent mutable families and one Bundle header/
recursive body by retained identity. [`OwnerStorage`](../src/assembly/owner_storage.cpp)
counts one shared recipe, one catalogue and one declaration per selection:
`shared_allocations = selections + 2`. Smart-pointer objects are already in their
containing object sizes. Control-block bytes remain outside the conservative
capacity census; independent request attribution includes their actual bytes.
The initial snapshot, capacity census, selected live/peak request counters and
cumulative request/work counters retain their separate meanings.

## Actual whole-owner comparison

Historical source `ac4ab9cf` binaries were verified against their original
manifests, copied before rebuilding, and executed on both Clang 15/GCC 11 at
one-worker bulk and two-worker direct. Their actual owner-adoption windows agree
with #363's recorded before costs. The new windows measure real adoption on the
tested source, with scratch/expected values prepared outside the window.

| Native fixture | Retained / scoped peak before → after | Adoption calls before → after | Cumulative requested bytes before → after |
| --- | --- | --- | --- |
| One accented-letter selection | 315,775 → 289,584 | 3,200 → 2,899 | 414,701 → 388,510 |
| Four copies, mixed exposure | 370,331 → 341,816 | 5,640 → 5,306 | 764,935 → 736,420 |
| Three one-byte copies, mixed exposure | 332,157 → 305,194 | 4,576 → 4,265 | 621,213 → 594,250 |

These are actual current-thread C++ requested bytes, not allocator metadata,
RSS, whole-process peak or performance qualification. Scoped owner-retained and
peak happen to coincide in these windows. Retained savings are 26,191 / 28,515 /
26,963 bytes. One recipe body contributes 25,735 / 28,059 / 26,507 bytes across
302 / 335 / 312 unique buffers. Those bodies are already in the whole-owner
totals; do not add them again.

Both compilers report projection size 1,304 → 800 bytes. The old 520-byte Bundle
header was embedded at offset 48 in each projection, so it was never an
additional request. The new shared payload/control-block request is 552 bytes.
Thus the observed header/ownership saving is `2*1304 - (2*800 + 552) = 456`
bytes, in addition to one recipe body. The capacity census instead saves 488
header bytes because it excludes the measured 32-byte control overhead.
Public independent copies acquire their own new shared root; no general claim
that every projection operation becomes cheaper follows.

Independent visitors verify exact selected-domain requests, unique body buffers,
one shared recipe identity across views and detached caller/other-owner identity.
They compare every Bundle field after caller-source destruction and before/after
allocation-free exposure staging. A test-only recipe alias outlives owner/view
destruction with the complete recipe and no mutable view backing. The last strong
alias releases the body; a weak recipe retains just its 552-byte selected request
and domain/bootstrap lifetime. Releasing it, then the weak domain, releases every
observed request. There is no ownership cycle.

## Validation and evidence

Native CPU/x64, C++17 Release, Clang/LLVM 15 and GCC 11 against the matched
repository fork/runtime. GPU backends/qualification are disabled. The GCC build
was configured fresh for this contribution, then incrementally rebuilt at the
final pin. CTest is serial; up to two independent matrix configurations per
compiler run concurrently on the shared host. Each final profile records clean
source and unchanged HEAD before/after.

| Local final-source scope | Result |
| --- | --- |
| Quick native build/backend, three CTests, placement arithmetic and capacity | Passed |
| Clang focused native/owner/viewport CTests | 16/16 passed |
| GCC native/owner/viewport CTests | 8/8 passed |
| Standalone projection/recipe suites, Clang and GCC | 4/4 each passed |
| Warm identity/order/path/address/text/UUID compatibility, Clang and GCC | 6/6 each passed; no server/persistent writes |
| Three owner cases × both compilers × 1/2 workers × bulk/direct × default/no-unroll | All 48 passed |
| Standalone exchange commit/fields at 1-bulk and 2-direct on both compilers | All eight passed |

The default owner cases retain their original guards after the recipe witness.
Added controls exercise public v1/v2 construction, independent public/domain
copies and assignment (including equal domains), self-copy/move and allocation-
free moves; every underlying allocation failure and reservation boundary; exact/
one-short work/bytes; populated inactive pools and full input/recipe payload;
caller/owner/shared/weak/domain death; retry, equal/mixed alternatives, repeated
forward/back publication and following ticks. No full regression, database,
GPU, timing/performance or complete machine-budget qualification is claimed.
Existing post-PASS Taichi cache-lock warnings remain in logs; no cache was cleaned.

The earlier frozen source `e1d6cb0b` passed quick CI and both standalone suites,
but its first GCC owner matrix reproduced an eight-byte partial-copy leak in
legacy validation. Its Clang focused run was interrupted after that finding.
Those logs and results are retained as failed/partial evidence, not final-source
qualification. The separate repair commit produces the tested `6be856c7` tree;
the final profiles are rerun from that clean exact public source.

Evidence is contributor-local, identified by the exact source, binary and
SHA-256 manifests. It is not remotely available or independently inspected
maintainer evidence. Hosted CI remains authoritative for its gates. Dependency
#363 and this contribution's hosted/disposition gates must be reconciled before
integration; no duplicate workflow is dispatched.

#363's native run 44149675384 now passes all 12 jobs, and smoke 44149675364
passes. Those status records are archived; its integration remains separate.

Retained evidence: `/tmp/hcp-recipe-shared-6be856c7-evidence.tar.gz`.
SHA-256:

```text
ce4c77245374a4054e4a9e14b3dfeac8c6694d3185b84947bef74a91688b3003
```

The bundle contains all seven final profiles, their verified manifests, source/
binary/toolchain/configuration/command records, retained before/after native
binaries, actual request/lifetime/work outputs, the historical owner-control log,
development and failed/partial-source logs, and dependency status records.
The archive listing and SHA-256 were checked after creation. Later handoff
changes are Markdown only relative to the tested production/test source.

Native indexed guards, catalogue/mixed-revision semantics, broader producer
adoption, full combined admission/machine accounting and #324 mathematics remain
separate. Taichi supplies structure; HCP retains the complete recipe meaning.
