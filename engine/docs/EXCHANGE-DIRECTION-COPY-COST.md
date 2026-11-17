# Duplicate forward/reverse exchange payload inspection

Contributor inspection under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343),
2026-11-17, awaiting review. Public tested source:
`657d0d87726ff4991590b3d6d0692a376acfefd1`.

Starts from main `45b5cf074b4941bf4addc1031a53d093fe986151`, which integrates
#358/#359. Normal merge `63905fc7` preserves source-accepted, unmerged #360
`444594ed12e0910623c2a7145fa6b45dec58e6cd`. Relative to that dependency,
production code is unchanged; the only executable change is a test-only cost
and lifetime witness. Runtime sharing is proposed below, not implemented.

## Current owner, readers and lifetime

[`ExchangeOwner`](../src/assembly/exchange_owner.cpp) adoption collects a complete
`LetterExchangeContract` for each selected occurrence, copies that entire
contract, swaps the copy's from/to exposure and outgoing/incoming vectors, then
retains both as separate `shared_ptr<const LetterExchangeContract>` objects.
Each snapshot owns the same full owner/study key, pinned recipe, source
definitions, paths, masses, nodes and ordered parent targets/ordinals/stored
masses/counts. Capacity can differ after copying; complete semantic records do
not. Repeated occurrences retain their own owners, and neither graph is a live
native mapping.

Direction is required state. Owner declaration selection checks the full owner
key and requested target, then compares the selected declaration's source
exposure with the committed projection. Same-exposure, unknown/nested/foreign
owners and invalid targets refuse. Readiness and generation validation precede
staging. The immutable initial recipe exposure is not current exposure after
publication.

[`ExchangeBindings`](../src/assembly/exchange_bindings.cpp) shares the selected
const snapshot. Capture uses its outgoing rows to verify current full native
handles, exact source mass and complete participation. The
[`coordinator`](../src/assembly/exchange_coordinator.cpp) consumes incoming rows
for prospective commands/maps. Both readers and owner selection must agree on
the chosen operation direction. The owner retains both snapshots across
successive preparations; selected bindings may extend one snapshot's lifetime.
No mutable alias exists on this path. Weak shared ownership can retain the
coallocated storage and reservation-domain allocator after the payload is
destroyed, until the last weak reference dies.

Native `field::UpdateCommand` and record handles carry execution operations,
coordinates, incarnations, exact mass/counts and related ledger information.
They do not carry complete HCP study/recipe/definition/parent pins or the
character/byte source identity. Taichi owns native structure; HCP must retain
that semantic information. The duplicate is the second immutable declaration
graph, not those semantic records themselves, native claims, generation guards
or candidate/committed publication state. This proposal does not remove native
maps or introduce another structural registry.

## Actual reverse snapshot requests

The test friend executes the same private copy constructor, from/to swap,
source-vector swap and `allocate_shared<const LetterExchangeContract>` used in
owner adoption. Measurement starts after the forward declaration, independent
expected full descriptors and optional reservation domain exist. It isolates
the retained reverse snapshot. Forward collection, owner declaration-table
growth, temporary binding validation, native backing and whole owner adoption
are outside this window.

Both unreserved and selected-domain paths run three fixtures: repeated accented
multibyte text at maximum entry identity, mixed-width text initially exposed as
parts, and one-byte English with one row on each side. Independent allocation
observation attributes every retained string/vector buffer, verifies distinct
backing from the forward graph, and checks each allocator's selected domain.
The shared-object request is found by containment in the observed allocation
table, not a guessed control-block size. Clang 15/GCC 11 agree on Linux x64:

| Reverse snapshot fixture | Requests | Cumulative / retained / scoped peak requested bytes | Nested payload blocks / bytes | Shared object request | Preparation work |
| --- | --- | --- | --- | --- | --- |
| Repeated multibyte | 68 | 5,368 / 5,368 / 5,368 | 67 / 5,040 | 328 | 5,623 |
| Mixed-width, initially parts | 80 | 6,304 / 6,304 / 6,304 | 79 / 5,976 | 328 | 6,557 |
| One-byte English | 26 | 2,046 / 2,046 / 2,046 | 25 / 1,718 | 328 | 2,309 |

Results match with and without the selected reservation domain. Its live-byte
report equals each retained total; the unreserved printed domain total is zero
because no domain exists. Every payload buffer plus the single shared request
accounts for the whole retained snapshot. The 296-byte contract object lies
inside that 328-byte request; its extent and the remaining coallocation overhead
are not additional requests. No guessed allocator rounding is included.

Preparation work includes the actual private copy plus two 296-unit object
charges associated with making and retaining the reverse snapshot. It excludes
the forward object's retention charge, declaration-table appends and binding
validation between those phases in actual adoption. The work units and C++
requested bytes measure different quantities and must not be added together.

After the last reverse strong reference dies, all nested payload requests are
gone and the weak reference retains just the 328-byte coallocation. On the
bounded path that request remains domain-charged and its allocator keeps the
domain alive after the caller drops its reference. Releasing the final weak
reference frees that request and releases the domain. All six cases finish
with zero observed live blocks/bytes and acquired/released requests balanced.

Counters observe current-thread C++ new/new[] requests, not arbitrary malloc,
allocator usable size, RSS, process/device memory or whole-owner peak. The
selected domain is created outside the window. These are actual current costs
and candidate reductions, not implemented savings or a total accounting policy.
Do not add them to the earlier leaf-removal measurements, which describe a
different source transition.

## Test-only sharing witness and bounded proposal

The witness also copies a const shared ownership alias and reads its existing
source sides in the explicitly opposite roles, without constructing another
descriptor graph. This alias/role/matching window makes zero C++ requests.
Canonical comparison covers full occurrence and definition pins, mass, node,
ordered parents, full targets, stored mass and counts. Exact matching work
passes; one-short work, unswapped roles, stale parent revision and foreign study
refuse. The one-byte case rejects wrong roles despite equal row counts. Row
count, mass, handle equality and SNode identity cannot select semantic direction.

After the original forward reference, projection and fixture die, the alias
still exposes the same complete rows in both roles. Releasing that alias
expires the original forward object, while the actual copied reverse snapshot
still matches the independently retained full descriptors/header. This proves
immutable storage/lifetime availability; it does not qualify a new native
exchange, admission policy or orientation adapter.

Proposed next implementation, subject to review: retain one immutable complete
payload per exact selected owner, with an explicit operation target/role on the
existing private generation-bound binding. Owner selection must validate full
owner identity, valid opposite target and current exposure before staging that
role. Capture must read the selected outgoing side and command construction
the selected incoming side. Both directions must use one coherent staged role;
changing a request must never reuse stale direction or previously valid intent.
Keep the public standalone contract's one-way from/to/outgoing/incoming meaning
and matching behavior. The private owner can orient its retained payload without
mutating a const graph or creating a general wrapper/registry.

Preserve full semantic pins, canonical order and caller permutation rules,
both exposure alternatives, repeated contributions, exact mass/counts,
stored-mass provenance, native claims, current/candidate maps, membership
catalogue, generation/readiness, and atomic publication. Do not share across
distinct owners or recipe revisions, infer orientation from cardinality, or
adopt broader public producers as part of this change.

Coordinate the owner constructor/selection, binding staging/capture, coordinator
incoming-side access, retained census and independent tests. Remove only actual
reverse graph construction/copy/retention charges and declaration entries;
preserve source, pin, association and direction validation work. Recalculate
binding object extent, table capacities, coallocation and selected-domain
effects rather than assuming a free direction field or predicting whole-owner
savings from this isolated window. Actual before/after owner measurements must
check shared census identity and every reached allocation/unwind boundary.

Implementation qualification must exercise repeated forward/back publication,
single/multiple owners and mixed initial exposures, equal-sized alternatives,
stale/foreign/nested/same-target refusal, exact/one-short work and byte gates,
generation/incarnation validation, preparation retry, detached/shared/weak
lifetime and following ticks across supported worker/compiler paths. The
existing native candidate/claim/placement/publication controls remain required.
Direct indexed guards, catalogue/mixed-revision changes, broader producer
adoption, full accounting and #324 mathematics stay separate.

## Source-pinned validation and handoff

Personally executed at clean public `657d0d87`, with exact recursive submodule
pins and matched CPU-only LLVM/Clang 15 fork/runtime recorded:

- `HCP_LOCAL_BUILD_JOBS=1 bash engine/scripts/run_local_ci.sh quick /tmp/hcp-exchange-direction-657d0d87-quick`:
  PASS. Native rebuild, actual CPU backend, three core CTests (2.62 seconds),
  632 independent placement interval samples and placement-capacity controls.
- `bash /tmp/hcp-exchange-direction-657d0d87-checks.sh focused`: the engine-build
  `structural_recipe_test` passes all 3,938 controls in 0.07 seconds. This is a
  pure value test despite its native engine-build configuration.
- `bash /tmp/hcp-exchange-direction-657d0d87-checks.sh clang` and `... gcc`:
  fresh Release standalone Clang 15/GCC 11 builds each pass all four projection,
  membership, structural-recipe and bootstrap-reservation CTests in 0.59/0.54
  seconds, without Taichi/PostgreSQL linkage. All producer measurements agree
  across compilers and the engine build. The Clang standalone and engine recipe
  binaries also have identical hashes.

The original 3,902 recipe controls remain, including all 448 actual factory
allocation-refusal points and existing work/byte guards. The new witness adds
36 checks across three fixtures and two allocator paths. No production or native
owner behavior changed relative to #360, and its accepted baseline was not
rerun merely for onboarding. This slice does not claim local full regression,
native compiler/owner exchange matrices, database, GPU, browser or Windows
qualification.

All final profiles preserve unchanged tested head/clean-source checks,
commands, configuration, submodules, complete logs and binary hashes. All
manifests and original binary hashes verify. Context retains development logs,
the test-only patch, parent job metadata and verified copies of both standalone
recipe binaries and the engine recipe binary for a future actual before/after
comparison. Known post-PASS offline-cache lock warnings remain in quick logs;
no cache or retained store was cleaned. Final notes change only five Markdown
files after the tested source.

Raw directories: `/tmp/hcp-exchange-direction-657d0d87-{quick,focused,clang,gcc,context}`.
Complete archive excluding build trees, with the three retained binaries:
`/tmp/hcp-exchange-direction-657d0d87.tar.gz`, SHA256
`08c6f716f83b24c0fdde7c500ab649b3174c1744ad470aafad13749bd1dc56e6`.
These are contributor-local results; hashes do not make the raw evidence
remotely available or independently inspected.

#360 native run [44109254971](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/44109254971)
now reports success in all 12 jobs, and its smoke passed. Its source review
[5941694377](https://github.com/Human-Cognome-Project/human-cognome-project/pull/360#pullrequestreview-5941694377)
stands; integration remains separate. Main `45b5cf07` combined native
44118550566 is still running at inspection. No duplicate workflow was requested.
This inspection's authoritative hosted CI and maintainer review remain pending.
Review this bounded candidate before implementing runtime sharing.
