# Exchange leaf provenance from retained source parents

Contributor implementation under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343),
2026-11-17, awaiting review. Public tested source:
`0750a2cdba07164a1dea9665302f58e07c9983f3`.

Implements the removal authorized by
[#359 review 5939931047](https://github.com/Human-Cognome-Project/human-cognome-project/pull/359#pullrequestreview-5939931047),
following the [actual inventory comparison](EXCHANGE-LEAF-INVENTORY-COST.md).
Starts from main `d1139a89`, preserving source-accepted, unmerged #359 `69edf33a`
and #358 `3b570a4f` through normal merge `e2730f98`. Their remaining hosted and
integration gates are separate; this branch does not label either dependency
integrated.

## Existing parent records own the complete meaning

[`LetterExchangeContract`](../src/assembly/exchange_contract.h) no longer owns
`ExchangeLeaf`, a separate leaf vector or a `leaves()` export. Collection and
reverse copying no longer construct/copy that second inventory. The existing
const outgoing/incoming accessors still expose complete canonical source rows
and ordered parents, without a replacement owning array, registry, range wrapper
or compatibility fallback.

The byte alternative is outgoing when `from()==Parts` and incoming when
`from()==Aggregate`. Each byte source's two-element occurrence path supplies
character/byte ordinals. Each direct parent supplies its ordinal, complete target
ToolRef and exact contribution. Reading those fields preserves the complete
ordered nibble provenance. Aggregate parents remain distinct from byte parents;
repeated definitions/bytes/nibbles are not deduplicated. Full owner/study/recipe
pins, source definitions, exact masses, nodes and stored-mass provenance remain.

The [collector](../src/assembly/exchange_contract.cpp) keeps all study/source
lookups, full-pin comparisons, target/exposure checks, byte-path/parent-count
checks, overflow/mass conservation and comparison-work calculation. `matches()`
still checks complete source/parent bijections, accepts caller permutations and
rejects missing/duplicate/foreign/stale or changed descriptors. Its comparison
bound is unchanged. Only actual leaf construction, pin copying, growth/relocation,
reverse-copy work and the removed 40-byte object member extent stop charging.

The [retained census](../src/assembly/owner_storage.cpp) counts the actual smaller
contract object, outgoing/incoming capacities and complete nested source/parent
keys once. Independent test visitors stop enumerating the removed buffers.
Declaration-copy equivalence now checks canonical source/parent order explicitly,
including full paths/pins/counts/node/stored mass. This preserves derived leaf
order while leaving the production caller-permutation rule intact.

Bindings still share immutable declarations; reverse copies remain independent
of their forward source. Borrowed parent references live only as long as their
declaration. Native claims, current/candidate maps, source ledgers, generation,
readiness and publication remain under their existing owners. Native record or
SNode identity does not replace HCP semantic pins or source ordinals. No native
layout, execution policy, numerical law or public producer adoption changes.

## Actual producer before/after

Verified Clang/GCC binaries pinned to public `87061d3b` execute the actual old
factories; #359's head differs only in Markdown. The original native owner binary
also matches that source's retained quick-CI binary manifest. All were copied
before replacement builds and rerun with complete logs on the unchanged matched
fork/runtime. These are historical executable measurements, not reconstructed
legacy allocation estimates.

The changed witness retains the same repeated/multibyte fixtures and actual
public v2 collection, bounded v2 collection and private reverse-copy paths.
Independent expected values come from the normalized pinned recipe, outside the
measurement window. Both compilers agree:

| Actual producer | Requests, before → after | Cumulative requested bytes | Retained requested bytes | Scoped peak requested bytes |
| --- | --- | --- | --- | --- |
| Repeated, public / bounded collect | 97 → 69 | 9,016 → 5,904 | 7,032 → 5,040 | 7,032 → 5,040 |
| Repeated, bounded reverse copy | 92 → 67 | 7,032 → 5,040 | 7,032 → 5,040 | 7,032 → 5,040 |
| Mixed multibyte, public / bounded collect | 117 → 82 | 14,706 → 8,856 | 10,290 → 6,840 | 11,442 → 7,992 |
| Mixed multibyte, bounded reverse copy | 110 → 79 | 8,466 → 5,976 | 8,466 → 5,976 | 8,466 → 5,976 |

Eight repeated leaves formerly retained a 1,280-byte vector request and 712 nested
pin bytes: 1,992 requested bytes removed. Ten mixed leaves formerly retained
2,560 vector bytes at collection capacity 16 or 1,600 at reverse-copy capacity 10,
plus 890 nested bytes: 3,450/2,490 removed. These components explain the observed
retained totals; do not add them to those totals again. Cumulative collection
requests also included replaced vector capacities, yielding 3,112/5,850 fewer
requested bytes. Peak is measured separately; matching retained reductions here
are fixture results.

The required parent pins retain their original 24/30 nested requests and 712/890
bytes. Eight/ten provenance rows are still read in the same canonical order.
Source-table capacities remain 4 for the repeated byte alternative, 8 for mixed
collection and 5 for its reverse copy. Full source/parent values still match
independent inputs after the forward declaration, projection and fixture die.
Borrowed reads allocate nothing and changed full parent target revisions refuse.

On this Linux x64 ABI the contract changes from 336 to 296 bytes. Its removed
40-byte vector header is inside that object, outside the value-return C++ request
window. It is not another heap request in the table above. Bounded collection
uses 21,129/28,733 preparation-work units; reverse copying uses 5,031/5,965.
The public factory's internal work meter is not exported; its printed zero is
the witness's unused external meter.

## Native owner request/census witness

The actual one-selection, two-byte owner fixture retains a forward and reverse
declaration, each formerly containing four leaves. Before and after executions
preserve the original independent request/census, all-failure-site, shared
lifetime, rejection/retry, stable-backing and 32 following-tick controls:

| Native owner fixture measure | Before | After |
| --- | --- | --- |
| Retained requested bytes | 320,845 | 318,773 |
| Scoped peak requested bytes | 320,845 | 318,773 |
| Retained capacity charge | 320,653 | 318,581 |
| C++ allocation/failure sites | 3,267 | 3,239 |

The 2,072-byte reduction is two removed 640-byte leaf vector requests, two
356-byte nested pin groups and two 40-byte headers in the actual shared contract
object allocations. The capacity and actual-request reductions agree; their
reports overlap and must not be summed. Every one of the 3,239 reached owner
allocation-failure sites reclaims its partial output. No native backing or
independently required mapping is removed to obtain these numbers.

Counters observe current-thread C++ new/new[] request lifetimes, not arbitrary
malloc, allocator rounding, RSS, native/device memory, whole-process peak or
latency. Logical factory windows exclude domain creation, result object storage,
expected inputs, projection preparation and owner shared/control-block creation.
The separate native owner witness observes its actual adopted C++ requests;
borrowed Harness/native backing and caller memory remain outside that window.
Neither report completes full B or combined admission accounting.

## Controls and qualification

The [value test](../tests/structural_recipe_test.cpp) keeps its original
context/UTF-8, repeated/multibyte, both-direction, complete pin/provenance/node,
mass/count, permutation, stale/foreign/duplicate, exact/one-short work and every
allocation-budget refusal control. Independent expected leaves exist only in
the test oracle, outside the measured factory; no owning leaf array remains in
production or returned declarations.

All 448 reached C++ allocation failures across six actual factory paths now
also refuse with scratch/output/domain live bytes back at zero. The suite passes
3,902 controls; fewer dynamic work/allocation-budget boundaries reflect actual
removed leaf storage/copy work, with the guard families preserved. Native
declaration snapshots/copies/moves, selected-domain ownership and shared-owner
lifetime still use independent allocation observation and complete canonical
source/parent comparisons.

Personally executed at clean public `0750a2cd`, matched CPU-only LLVM 15 fork
with exact recursive submodules recorded in the evidence:

- `bash engine/scripts/run_local_ci.sh quick /tmp/hcp-exchange-leaf-0750a2cd-quick`:
  PASS. Native rebuild, actual CPU, three core CTests, 632 independent placement
  interval samples and placement capacity.
- `flock engine/build/.local-ci.lock bash /tmp/hcp-exchange-leaf-0750a2cd-checks.sh focused`:
  all 16 CTests passed in 454.11 seconds, including structural recipe, projection,
  membership value/native, builder, all four prepared-owner and all four observer
  profiles, workbench, origin provenance and complete snapshot. Four further
  CLI invocations pass single/multiple-owner exchange at one-worker bulk and
  two-worker direct paths, including all lifetime/failure/retry controls.
- `bash /tmp/hcp-exchange-leaf-0750a2cd-checks.sh gcc`: a fresh GCC 11 Release
  wrapper against the same matched Clang-built fork passed eight selected
  CTests in 137.23 seconds, followed by those same four exchange invocations.
  Complete primary owner request/census and 3,239 failure-site results agree
  with Clang; both single and multiple owners preserve following ticks.
- `bash /tmp/hcp-exchange-leaf-0750a2cd-checks.sh clang-values` and `... gcc-values`:
  fresh standalone builds each passed all four projection/membership/structural
  recipe/bootstrap-reservation CTests in 0.56/0.51 seconds without Taichi/
  PostgreSQL linkage. Every reported producer measurement agrees across both
  compilers and the engine build.

All final profiles record unchanged head/clean-source checks, commands,
configuration, submodule pins, complete logs and binary hashes. Historical
before binaries/manifests/executions and development observations are retained
separately. Known post-PASS offline-cache lock warnings remain in the logs.
All per-file manifests verify. Final notes change only six Markdown files
after the tested source.

Raw final directories:
`/tmp/hcp-exchange-leaf-0750a2cd-{quick,focused,gcc,clang-values,gcc-values,context}`,
plus `/tmp/hcp-exchange-parent-leaf-{before,development}`. Complete archive,
excluding build trees but retaining the before binaries:
`/tmp/hcp-exchange-leaf-0750a2cd.tar.gz`, SHA256
`bd945cd896dff8b9649ab9b2c96b00479b6d6318ee2183a7967114e3cd82297d`.
These are contributor-local executions, not independently inspected remote raw
evidence. Full hosted regression/compiler/database matrices, GPU, browser and
native Windows were not run locally for this slice.

#358 native 44089456571 now reports success. #359 native 44099141583 remains
running at inspection; their smoke gates passed. Both dependencies are still
unmerged. This implementation's authoritative hosted qualification is separate;
no duplicate workflow is dispatched.

Review and authoritative hosted CI remain pending. Direct indexed guards,
catalogue/mixed-revision adoption, broader public producer migration, full
accounting and #324 mathematics remain separately scoped.
