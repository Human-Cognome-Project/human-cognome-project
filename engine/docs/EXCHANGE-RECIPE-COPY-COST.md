# Duplicate immutable recipe backing in exchange projection views

Historical inspection at the pinned source below. The subsequent
[owner-local sharing implementation](EXCHANGE-RECIPE-SHARING.md) supplies the
measured removal, coordinated admission/census and value/lifetime qualification.
The separate copies and candidate-only costs below describe the before source.

Contributor inspection under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343),
2026-11-17, awaiting review. Public tested source:
`ac4ab9cf3690a24004532674fe509e3bfb27c789`.

Starts from main `ad38b6cd`, which integrates #361. Normal merge `e69742d1`
preserves source-accepted, unmerged #362 `2cdf2ea8`. Production is byte-identical
to #362; this slice adds only a test witness and a bounded proposal. Recipe
sharing is not implemented.

## Required meaning and actual owner

The private [exchange owner](../src/assembly/exchange_owner.cpp) retains two
[`LetterProjection`](../src/assembly/letter_projection.h) objects: committed and
prospective. Each embeds its own complete `warm::Bundle recipe_`. Initial
adoption copies the caller projection into the owner's domain. Spare construction
then invokes the same domain-aware projection copy, including a second deep
`projection_recipe` copy of all recipe tables, text, addresses and paths.

That recipe is immutable initial source, not current exposure. It retains the
bundle selection/revision, owner/creator attribution, tool pins/epochs/authors/
schemes/notations/masses, ordered parent targets/ordinals/stored masses/counts,
upward memberships, geometry, conceptual nodes, occurrence identities, initial
selections, contacts and focal fields. All remain required HCP meaning.

`prepare_exposure` reads recipe tools/pins while updating the spare's separate
live exposures, occurrence activity, structural-input selection exposures and
active/spare parent pools. It does not update the recipe. Publication swaps the
two complete projection owners, retaining both object lifetimes. The study UUID
stays stable. No requirement for two independent copies of the immutable Bundle
follows from the requirement for independent mutable projection state.

The public `recipe()` accessor returns a const reference valid while its owning
projection remains alive. Current adopted views own independent deep copies and
outlive the caller source. Borrowing from the caller would violate that boundary.
Borrowed pointers also cannot survive owner/view teardown. A future shared recipe
must have an explicit owner lasting through both views and any permitted aliases;
aliasing an entire mutable projection is not a recipe-only lifetime solution.

## Native overlap

The narrower [structural handoff](../src/assembly/study_input.h) carries encoded
definition IDs, exact mass, centroid/components, parent value/ordinal/count,
occurrence IDs, node association and current selections. Native updates/claims
carry execution handles, incarnations, exact mass/counts and ledger information.
Neither that handoff nor Taichi layout/tree/cell identity supplies the complete
pinned Bundle, tool revisions, attribution, notation, stored-mass provenance or
original semantic construction. See the
[native instance inspection](NATIVE-INSTANCE-INSPECTION.md).

Taichi owns native structure; HCP must retain this meaning. The candidate is the
second identical immutable Bundle, not the recipe itself, current/candidate maps,
exposure state, live parent pools, native claims or publication protocol. No
catalogue lookup, global deduplication or structural registry is proposed.

## Actual adopted-buffer witness

The existing single-owner, four mixed-exposure and three one-byte-copy native
fixtures now run a recipe-family inspection before their original controls. A
dedicated `--case exchange_recipe_cost` runs just those three inspections, without
repeating their accepted allocation-failure/publication baseline. Default owner
cases still run every original guard after the new witness; hosted placement
suites therefore discover it without another CTest target.

Observer scratch, expected values, mutated comparator controls and a detached
caller projection are prepared before measurement. The window measures actual
owner adoption, not a reconstructed recipe factory. The caller projection is
then destroyed. An independent current-thread C++ allocation table attributes
each committed/spare recipe buffer's actual request and selected domain, verifies
matching capacities, rejects any within-family duplicate or cross-view alias,
and accounts for its bytes once. Inline/zero-capacity storage makes no heap
request. Complete value comparison uses every Bundle field listed above; changed
definition revision, parent-target revision, geometry or source node refuses even
with the same outer bundle pin. Those mutations are comparator controls, not
catalogue/mixed-revision adoption experiments.

Clang 15/GCC 11 agree on Linux x64:

| Actual fixture | Recipe buffers per view | Recipe requested bytes per view | Whole owner retained / scoped peak requested bytes | Whole adoption calls / cumulative requested bytes |
| --- | --- | --- | --- | --- |
| One accented-letter selection | 302 | 25,735 | 315,775 / 315,775 | 3,200 / 414,701 |
| Four copies, mixed initial exposure | 335 | 28,059 | 370,331 / 370,331 | 5,640 / 764,935 |
| Three one-byte copies, mixed exposure | 312 | 26,507 | 332,157 / 332,157 | 4,576 / 621,213 |

Each fixture retains two such independent recipe families. One family identifies
candidate duplicate backing; it is already included in whole-owner totals.
Do not add those reports together. These fixture recipes include the retained
definition universe, including definitions not currently exposed; this is not a
per-live-body or general production memory estimate.

The Bundle header is 520 bytes at offset 48 inside each separately observed
1,304-byte projection allocation. It is not a separate heap request. A future
sharing implementation would change projection extent, introduce/retain actual
shared-payload ownership and its control block, and require revised census
identity. Subtracting 25,735–28,059 plus 520 without measuring those effects would
not establish achieved savings or a complete owner budget.

Reading both logical uses through one existing const recipe allocates nothing.
Staging the opposite exposure also allocates nothing: committed and spare live
exposures differ while both recipe values, original selection exposures and every
recipe buffer pointer/capacity stay unchanged. Native state remains identical.
This establishes availability and immutability, not a new runtime sharing policy.

Owner teardown releases both recipe families and projections. The selected
domain object expires; its weak control block and intrusive bootstrap tracker
remain until final weak release. All observed C++ requests then reclaim with zero
live blocks/bytes and balanced acquisition/release. This witness retains no
borrowed recipe reference beyond its owning view's lifetime.

## Bounded proposal for review

Retain one immutable pinned Bundle for the two views belonging to one exact
exchange owner. Use ordinary shared const ownership through the existing
`LetterProjection`/owner boundary; keep `recipe()` const access and all required
meaning. Introduce no generic wrapper, global pin registry or catalogue migration
policy. Do not deduplicate across distinct owners or revisions merely because
their outer pins compare equal.

Use an explicit private owner-spare construction path to reuse that owner-owned
recipe. Initial adoption must still detach from caller storage. Public value
copies and explicit domain copies must preserve their current independent values,
allocator selection, source-death behavior and exception cleanup; equal allocator
domains alone do not prove a common semantic owner. Do not turn a default public
copy into an alias of a caller's selected-domain storage.

Keep exposures, published occurrence/activity state, both parent pools, structural
handoff state, current/candidate maps, bindings, membership catalogue, native
claims, generations and atomic publication separately owned as today. A shared
recipe's initial selection exposure must never substitute for committed exposure.
No new native tree, layout rebuild, kernel policy or force law follows.

Coordinate constructor/copy/move lifetime, retained census and all independent
visitors. Recalculate actual copy/allocation work and the initial/spare snapshot
demands in [adoption accounting](ADOPTION-ACCOUNTING.md), removing only operations
and backing genuinely eliminated. Preserve inspection, full-pin/association and
live-exposure validation. A smaller recipe copy does not complete combined
borrowed-resource or B/admission accounting.

Before implementation acceptance, measure actual owner before/after including
embedded header changes, shared object/control-block requests, domain lifetime,
temporary peak and census deduplication. Preserve v1/v2 standalone and selected-
domain copies, exact/one-short byte/work gates, every reached allocation failure,
source/owner death, partial-copy unwind, weak release, rejection/retry, repeated
forward/back publication, mixed/equal alternatives and following ticks across
supported worker/compiler paths. Current owner projection pools, claims and
publication state remain required. Indexed guards, catalogue/mixed revisions,
broader producer adoption, full accounting and #324 remain separate.

## Source-pinned qualification

Personally executed at clean public `ac4ab9cf`, with exact recursive submodules
and the matched CPU-only LLVM/Clang 15 fork/runtime recorded:

- `HCP_LOCAL_BUILD_JOBS=1 bash engine/scripts/run_local_ci.sh quick /tmp/hcp-recipe-inspection-ac4ab9cf-quick`:
  PASS. Native rebuild, actual CPU, three core CTests, 632 independent placement
  interval samples and placement capacity.
- `bash /tmp/hcp-recipe-inspection-checks.sh focused`: two native inspection runs
  at one-worker bulk and two-worker direct, each covering all three fixtures,
  passed on Clang. The existing full `exchange_owner` case also passed at
  one-worker bulk, including all 3,200 reached allocation failures, exact/one-short
  setup/work/byte gates, rejection/retry, shared/weak lifetime and following ticks.
- The same script with `gcc`: a fresh GCC 11 Release native wrapper against the
  same matched fork passed both three-fixture inspections and the existing full
  owner case at two-worker direct. Actual mode tags and identical measurements
  across all four dedicated runs were verified. Original guards remain enabled
  on default owner paths; the inspection-only CLI flag explicitly avoids an
  unnecessary repetition of their accepted baseline.

All three final profiles preserve unchanged tested head/clean-source checks,
commands, toolchain/configuration, submodules, complete logs and binary hashes.
Every manifest and original binary hash verifies. Context retains both qualified
native executables, development observations, source-tree equivalence and parent
job metadata. Production remains byte-identical to #362. Final notes change only
six Markdown files after the tested source.

Raw directories: `/tmp/hcp-recipe-inspection-ac4ab9cf-{quick,focused,gcc,context}`.
Complete archive excluding build trees, with both retained native executables:
`/tmp/hcp-recipe-inspection-ac4ab9cf.tar.gz`, SHA256
`b3509e2c351672f5b2c66514285af3f37d4b4b9bc9d73ff38afc87fad3fd9b25`.
These are contributor-local results; hashes do not make raw evidence remotely
available or independently inspected. Known post-PASS offline-cache lock warnings
remain; no cache or retained store was cleaned.

This scope does not rerun the unchanged standalone value suites, full owner-mode/
regression/compiler/IR/database matrices, GPU, browser or Windows qualification.
Counters exclude arbitrary malloc, allocator rounding, RSS, process/native/device
peak, exception-runtime handling and borrowed execution. Candidate family costs
are not implemented savings or complete resource admission.

#362 native [44137874103](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/44137874103)
now passes all 12 jobs, and its smoke passed. Its source review
[5943526107](https://github.com/Human-Cognome-Project/human-cognome-project/pull/362#pullrequestreview-5943526107)
stands; it remains unmerged at inspection, with main still `ad38b6cd`.
No duplicate workflow was dispatched. Review this bounded candidate before any
runtime sharing implementation; this contribution's authoritative hosted CI and
maintainer review remain pending.
