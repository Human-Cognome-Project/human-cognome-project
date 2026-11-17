# Exchange leaf inventory already represented by source parents

Contributor inspection under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343),
2026-11-17, awaiting review. Public tested source:
`87061d3b8519bd7b5c12d1136d10bae20bbd1e39`.

Starts from main `67861a37` and explicitly preserves source-accepted, unmerged
[#358](https://github.com/Human-Cognome-Project/human-cognome-project/pull/358)
at `3b570a4f`, accepted by review 5939303910. Normal merge `b59a54c3` retains
both ancestries and places the current maintainer handoff before the older
contributor entry. Production is byte-identical to #358. This contribution
adds a test-only ownership/cost witness and proposes a bounded removal; the
leaf inventory is still built and charged.

## One concrete duplication

[`LetterExchangeContract`](../src/assembly/exchange_contract.h) owns complete
outgoing/incoming `ExchangeSource` rows. The byte alternative includes each
occurrence's complete key, definition, exact mass, node and ordered direct
parents, including full target ToolRefs, ordinals, stored mass and contribution.
The same contract separately owns a flat `ExchangeLeaf` vector copied from
those byte parents by [collection](../src/assembly/exchange_contract.cpp).

| Leaf field | Existing source-parent authority |
| --- | --- |
| `character` | Byte source `occurrence.source.path[0]` |
| `byte` | Byte source `occurrence.source.path[1]` |
| `nibble` | Direct parent `ordinal` |
| `definition` | Direct parent complete `target` ToolRef |
| `count` | Direct parent exact `contribution` |

Choose the contract's byte side: outgoing when `from()==Parts`, incoming when
`from()==Aggregate`. Walk canonical source rows and each ordered parent.
Do not flatten the aggregate's direct parents: aggregate and byte field ledgers
have distinct meanings. Preserve repeated byte/nibble occurrences and exact
counts rather than deduplicating definition objects or inferring counts.

Collection owns every leaf pin independently: scope string, address vector
and revision string. The owner also creates a reverse declaration through the
actual private copy constructor and nonallocating side/exposure swaps. That
copy owns its own leaf inventory and source-parent pins. The
[owner](../src/assembly/exchange_owner.cpp) retains both immutable declarations;
bindings share const ownership. Complete values survive projection/fixture
teardown and a reverse declaration survives destruction of its forward source.
Borrowed parent references are valid only while that declaration stays alive.

## Native overlap and the semantic boundary

The source audit finds `leaves()` reads only in retained census and test/evidence
readers. [`ExchangeBindings`](../src/assembly/exchange_bindings.cpp) validates
full source descriptors and ordered parents against current native participation;
[`ExchangeCoordinator`](../src/assembly/exchange_coordinator.cpp) emits incoming
parent contributions directly from `source.parents`. Neither execution path reads
the flat leaf vector. `matches()` compares complete source/parent bijections,
including revisions and stored-mass provenance; the leaf vector is not its authority.

Native [`UpdateCommand`](../src/field/prepared_update.h) identifies current or
pending native records, ordered ledger positions, exact source counts and shares.
Definition identities remain the source adapter's responsibility. A native
record/index or SNode does not replace a complete HCP ToolRef or conceptual
character/byte ordinal. The [native inspection](NATIVE-INSTANCE-INSPECTION.md)
distinguishes those authorities and their lifetimes. This candidate reuses the
existing semantic parent records while leaving native structure and execution
under their current owners. It proposes no structural registry, cache, native
record reconstruction or new semantic rule.

Current [census](../src/assembly/owner_storage.cpp) correctly counts both owned
representations. They are distinct allocations today; merely reporting one
would undercount. Native claims, candidate/committed maps and publication state
are separate required ownership and are not selected for removal.

## Actual producers and observed cost

The [witness](../tests/structural_recipe_test.cpp) runs six actual factories:
public v2 collection, bounded v2 collection and the private reverse-copy path,
each for a repeated four-byte construction (eight ordered nibble leaves) and
a mixed multibyte five-byte construction (ten leaves). No reconstructed row
allocation estimate replaces a producer. Independent expected leaf values come
from the normalized pinned recipe; all expected/source/domain construction is
outside the allocation window.

Clang 15 and GCC 11 agree on current-thread C++ request measurements:

| Construction | Requests | Cumulative requested bytes | Retained requested bytes | Scoped peak requested bytes |
| --- | --- | --- | --- | --- |
| Repeated, public / bounded collect | 97 | 9,016 | 7,032 | 7,032 |
| Repeated, bounded reverse copy | 92 | 7,032 | 7,032 | 7,032 |
| Mixed multibyte, public / bounded collect | 117 | 14,706 | 10,290 | 11,442 |
| Mixed multibyte, bounded reverse copy | 110 | 8,466 | 8,466 | 8,466 |

Independent pointer attribution identifies the selected retained buffers:

| Inventory | Rows / capacity | Extra full-pin requests / bytes | Leaf vector request | Selected buffers total |
| --- | --- | --- | --- | --- |
| Repeated collect / reverse copy | 8 / 8 | 24 / 712 | 1,280 | 1,992 |
| Mixed multibyte collect | 10 / 16 | 30 / 890 | 2,560 | 3,450 |
| Mixed multibyte reverse copy | 10 / 10 | 30 / 890 | 1,600 | 2,490 |

The parent pins have equal 712/890 nested requested bytes with distinct backing;
they remain required. A leaf row occupies 160 bytes on this Linux x64 ABI. The
40-byte leaf-vector header is inside the 336-byte contract object, not another
request in the measured value-return window. Row extents are already inside
the vector request; do not add them again. Reverse copy reserves exactly the
leaf count, whereas collection grows geometrically. These are current candidate
costs, not realized savings or predicted construction peak reductions.

The test-only nested traversal visits identical full pins/ordinals/counts with
zero C++ allocations. Both representations and descriptor matching remain exact
after the forward declaration, projection and fixture die. A changed direct
parent target revision refuses matching even while the flat inventory is
unchanged, confirming that full parent pins remain necessary.

Bounded collection uses 24,889/34,973 preparation-work units; reverse copying
uses 7,016/8,446. Public collection's internal meter is not exported; its printed
zero is the witness's unused external meter. Selected-domain retained bytes
agree with actual retained requests in all bounded cases; teardown releases
every tracked block and returns the domain to zero live bytes.

Counters observe current-thread new/new[] request lifetimes, not arbitrary
malloc, allocator rounding, RSS, native/device memory, process peak or latency.
The value result object, domain creation, owner shared objects/control blocks,
projection preparation and expected inputs lie outside this window. This does
not complete full B/combined admission accounting.

## Bounded proposal — awaiting review

Retain all complete outgoing/incoming descriptors and their canonical byte
parent rows. Remove the separately stored leaf vector and read its semantic
fields from those existing rows through current const accessors. Coordinate
the contract/export, collector/reverse copy, retained census and test readers.
No owning temporary leaf array, new registry or generic range wrapper is needed.
The current factory accepts no external leaf table; add no compatibility
fallback solely for this cleanup.

Preserve full owner/study/recipe pins, occurrence paths, both exposure directions,
complete target revisions, exact masses/counts, node and stored-mass provenance,
ordered/repeated contributions and descriptor permutation rules. Keep the
complete bijection/comparison-work validation and stale/foreign/duplicate refusal.
Preserve detached and binding-shared lifetime and all native claim/publication
boundaries. No alternative becomes live merely by appearing in a declaration.

Adjust actual leaf construction, relocation, reverse-copy and census charges
only after removing storage. Preserve source/traversal/association validation,
exact/one-short work/allocation refusal and GCC unwind controls. Measure the
changed public/bounded/reverse producers, including actual object/header padding,
capacity, retained and peak effects. Run native owner/exchange/shared-lifetime
controls for an implementation; this comparison does not claim to implement it.

## Qualification

Personally executed at clean public `87061d3b`, matched CPU-only LLVM 15 fork
with exact recursive submodules recorded in the evidence:

- `bash engine/scripts/run_local_ci.sh quick /tmp/hcp-exchange-leaf-87061d3b-quick`:
  PASS. Native rebuild, actual CPU, three core CTests, 632 independent placement
  interval samples and placement capacity.
- `flock engine/build/.local-ci.lock bash /tmp/hcp-exchange-leaf-87061d3b-checks.sh focused`:
  the engine-build `structural_recipe_test` passed in 0.05 seconds, including all
  4,048 structural recipe/declaration controls and six actual factory witnesses.
  This remains a pure value test despite its engine-build configuration.
- `bash /tmp/hcp-exchange-leaf-87061d3b-checks.sh clang` and `... gcc`:
  fresh standalone Clang 15/GCC 11 builds each passed all four projection,
  membership, structural-recipe and bootstrap-reservation tests in 0.54/0.50
  seconds, without Taichi/PostgreSQL linkage. All six factory measurements agree
  across both compilers and the engine build.

Original graph/context/UTF-8, foreign/stale/duplicate descriptor, source-count,
stored-mass, node/ordinal, permutation, reverse-copy, exact/one-short work and
every reached allocation-budget refusal controls remain. The new witness adds
full parent-target revision refusal and detached ownership/attribution checks.
No production source, quota or retained census changes relative to #358.

All profiles preserve unchanged head/clean-source checks, commands, configuration,
submodule pins, complete logs and binary hashes; all manifests verify. Copied
Clang/GCC producer binaries were verified against those manifests and retained
for a future actual before/after comparison. Development observations are kept
separately from the frozen runs. Final notes change only five Markdown files
after the tested source.

Raw directories: `/tmp/hcp-exchange-leaf-87061d3b-{quick,focused,clang,gcc,context,before}`.
Complete archive, excluding build trees but including the verified binaries:
`/tmp/hcp-exchange-leaf-87061d3b-complete.tar.gz`, SHA256
`83788b330ccbe9d52adc70bd22d0c6c790dbb4d7ecf41d86612085e27db8fb7b`.
This is contributor-local evidence, not independently inspected remote raw
evidence. Full native/compiler/database matrices, GPU, browser and native Windows
were not rerun locally for this test-only comparison.

#358 native run 44089456571 remains separate qualification; its build, projection,
database and all compiler jobs report success, with two regression shards still
running at inspection. Its smoke passed. No duplicate workflow is dispatched.

Review and authoritative hosted CI remain pending. The #358 source dependency
is not labelled integrated. Direct indexed guards, catalogue/mixed revisions,
public producer migration, complete accounting and #324 mathematics remain
separately scoped.
