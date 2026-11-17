# One immutable exchange payload per selected owner

Contributor implementation under [#343](https://github.com/Human-Cognome-Project/human-cognome-project/issues/343),
2026-11-17, awaiting review. Public tested source:
`faa7fc8ed260f093ddd9987726cf25b4bec0c293`.

Implements the bounded sharing authorized in
[#361 review 5942373407](https://github.com/Human-Cognome-Project/human-cognome-project/pull/361#pullrequestreview-5942373407),
following the [reverse graph inspection](EXCHANGE-DIRECTION-COPY-COST.md).
Starts from main `3fd1936e`, which integrates #360 and its dependencies. Normal
merge `c50bb1f8` retains source-accepted, unmerged #361 `71d0c80d`. Its remaining
hosted/integration gates are separate from this contribution's qualification.

## Direction belongs to the pending binding

[`ExchangeOwner`](../src/assembly/exchange_owner.cpp) now retains one complete
immutable `LetterExchangeContract` per exact selected owner. Adoption collects
and validates the initial declaration, retains it, and validates initial native
participation as before. It no longer copies the full graph, swaps the copied
headers/vectors, allocates a second shared object or appends its second entry.
Different occurrences still own different payloads, even with identical
definitions and revisions. There is no cross-owner/revision sharing.

Selection checks the full supplied owner key and valid requested target, then
compares the required source exposure with the committed projection's live
exposure. It rejects foreign/unknown/nested owners, same-exposure requests and
invalid targets before projection/binding staging. Readiness and generation
checks still precede selection. Initial recipe exposure, row counts, mass and
native identity do not determine the requested operation.

The existing private [`ExchangeBindings`](../src/assembly/exchange_bindings.h)
stores that explicit target with the shared const payload. If the target is
the payload's original `to()`, outgoing/incoming retain their original roles;
the opposite target exchanges those roles. Capture and the
[`coordinator`](../src/assembly/exchange_coordinator.cpp) use that same staged
target for source validation and incoming commands. Capture also checks the
current committed exposure with a prepaid selection lookup. No const graph is
mutated or rebound to another owner's semantic identity.

Public standalone `from()/to()/outgoing()/incoming()` and `matches()` remain
one-way and unchanged. Standalone binding adapters explicitly select their
supplied declaration's `to()`. The owner uses its binding's selected roles;
it must not reinterpret the immutable public header as current exposure.

The complete owner/study/recipe/definition/parent pins, canonical source paths,
ordered parents, stored mass, exact counts/mass, node bindings, both alternatives
and repeated contributions remain in the retained graph. Native claims,
membership catalogue, current/candidate maps, placement, generation/readiness
and atomic publication keep their existing owners and checks. Failed preparation
still clears pending authority before work, leaves binding validity false and
the coordinator consumed/unsealed, and preserves committed state for retry.
There is no new range wrapper, structural registry, scheduler or execution owner.

## Work, census and actual storage

The reverse graph's private copy and two object-work charges are removed, along
with its actual shared retention and table append/relocation work. The forward
collector's full source/pin/association validation and retention charges remain.
Declaration selection explicitly charges its target/source role check. Capture
prepays one selection unit per possible lookup row plus a fixed role comparison;
existing source, membership, ledger and command work stays charged. Work limits
are recalculated from actual paths, not reduced to a pointer-equality test.

The [retained census](../src/assembly/owner_storage.cpp) visits each owner payload
once. Binding pointers are inside the binding object and do not count the same
graph or shared allocation again. Independent visitors still attribute every
retained table, path, address and UUID buffer to actual requests and the selected
domain. All six map stores, both projections, candidate commands and catalogue
remain included.

On this Linux x64 ABI both Clang 15/GCC 11 report the binding as 376 bytes after
adding the target enum; it occupies existing tail padding. The pre-change
compiled header probe also reports 376. Owner/coordinator/contract extents remain
608/680/296 bytes and a shared-pointer entry is 16 bytes. This is measured ABI
behavior, not a portable promise that direction has no storage cost.

Verified historical native executables provide the before results: Clang source
`657d0d87` and GCC source `0750a2cd`. Their engine/owner-test scope is identical to
reviewed #361; only the unrelated recipe inspection test differs. Original binary
manifests were verified before copying or rebuilding. Both executables were run
at one-worker bulk and two-worker direct against the unchanged matched fork and
runtime, with full logs retained. This is an actual before/after comparison,
not reconstruction of an old owner from its census formula.

| Native owner fixture | Before | After | Reduction |
| --- | --- | --- | --- |
| One selection: retained / scoped peak requested bytes | 318,773 / 318,773 | 315,775 / 315,775 | 2,998 / 2,998 |
| One selection: retained capacity charge | 318,581 | 315,615 | 2,966 |
| One selection: C++ allocation/failure sites | 3,239 | 3,200 | 39 |
| Four mixed selections: retained / scoped peak requested bytes | 382,323 / 382,323 | 370,331 / 370,331 | 11,992 / 11,992 |
| Four mixed selections: retained capacity charge | 381,939 | 370,075 | 11,864 |
| Four mixed selections: C++ allocation/failure sites | 5,793 | 5,640 | 153 |

Each accented-letter reverse snapshot formerly owned 2,654 nested requested bytes
and one 328-byte shared-object request, including the 296-byte object extent.
The table loses 16 retained bytes per selection in these fixtures. Thus
`2,654 + 328 + 16 = 2,998` explains the observed one-selection reduction; four
copies explain 11,992. The capacity reduction excludes the 32-byte coallocation
overhead removed with each shared snapshot. These decompositions overlap the
whole-owner totals and must not be added to them again.

A separate table-only replay of the unchanged shared-pointer vector append path
compares the old two entries with the new one per selection. Null pointees
exclude graph costs. Both compilers report capacity 2→1 / 8→4 for one/four
selections, retained table bytes 32→16 / 128→64, and cumulative table requests
48→16 / 240→112. Its peaks are separate phase windows, not whole-owner peaks.
Actual post-change owner witnesses also report capacities 1/4. This separates
table growth from shared graph storage without inventing allocator overhead.

The selected domain's cumulative allocation-work charge falls
417,555→414,541 / 776,831→764,775. These units are requested allocation bytes,
including temporary/replaced buffers. Shared setup's logical preparation work
falls 578,245→574,972 / 1,273,537→1,260,457, including the new exposure lookup
checks. Allocation work, logical preparation work, capacity and requested-byte
reports measure overlapping or different surfaces; they are not additive.

## Lifetime, refusal and publication witnesses

The retained owner witness independently accounts for selected requests, then
keeps two aliases to the same payload after owner teardown. Complete canonical
values still match a separately retained standalone declaration. Releasing one
alias preserves the other; releasing both destroys the payload while a weak
reference retains its shared coallocation and allocator/domain relationship.
After catalogue release and the caller's domain release, that weak declaration
also keeps the domain/control block and intrusive bootstrap tracker alive.
Final declaration-weak release expires the domain object; its own weak reference
keeps the bootstrap allocations until that final weak reference is released.
All observed requests then reclaim with zero remaining blocks/bytes.

Existing single/multiple-owner tests still compare full ordered sources/parents
against newly derived directional declarations, retain one pointer across both
directions, and check independent owners have distinct payloads. All current
work/byte/setup/bootstrap boundaries and each reached C++ allocation failure
refuse with complete unwind and exact retry. Existing stale/foreign/unknown/
nested/same-target/invalid-target, generation/incarnation, partial preparation,
hierarchy sealing, publication failure, survivor, report and following-tick
controls remain active.

The new `exchange_owner_equal` native fixture uses three one-byte English copies
with mixed initial exposures; two untouched copies retain both field families.
Every alternative has one body and mass two, yet the wrong explicit role refuses
capture. Sixteen forward/back publications preserve the other copies and backing,
check exact/one-short staging/capture/command work, invalidate previous intent on
same-target refusal, deduplicate census aliases and complete following ticks.
Its 4,576 reached allocation failures also unwind. Together the three fixtures
exercise 13,416 actual failure sites per worker/path/LLVM profile.

Development logs preserve two corrected new-test expectations: weak declaration
storage also retains the bootstrap/domain requests created inside owner adoption,
and exact staging comparisons require the same normalized spare-parent partition
already used by the existing staging sweep. No original guard or quota is relaxed.

## Source-pinned validation and limits

Personally executed at clean public `faa7fc8e`, using the exact recursive
submodules and matched CPU-only LLVM/Clang 15 fork/runtime recorded in evidence:

- `HCP_LOCAL_BUILD_JOBS=1 bash engine/scripts/run_local_ci.sh quick /tmp/hcp-exchange-shared-faa7fc8e-quick`:
  PASS. Native rebuild, actual CPU, three core CTests, 632 independent placement
  interval samples and placement capacity.
- `HCP_SOURCE_PIN=faa7fc8ed260f093ddd9987726cf25b4bec0c293 bash /tmp/hcp-exchange-shared-checks.sh focused`:
  all 16 Clang CTests passed in 449.36 seconds, including projection, membership
  value/native, structural recipe, admission builder, all four prepared-owner
  and all four observer profiles, workbench, origin and snapshot.
- The same script with `gcc`: a fresh GCC 11 Release native wrapper passed eight
  selected CTests in 134.91 seconds against the same matched Clang-built fork.
- Both native profiles additionally passed the three owner fixtures across
  workers 1/2, bulk/direct and default/disabled LLVM unroll: 48 CLI runs. Eight
  further one-worker bulk/two-worker direct `exchange_commit`/`exchange_fields`
  runs passed, preserving standalone one-way adapter behavior. Actual backend/
  worker/path/LLVM tags, completed case results and identical owner measurements
  were independently verified across every owner run.
- The script with `clang-values` / `gcc-values`: fresh standalone builds each
  passed all four CTests in 0.55/0.53 seconds without Taichi/PostgreSQL linkage,
  retaining all 3,938 recipe controls and 448 factory allocation-refusal points.
  The historical reverse factory remains a copy/lifetime control; current owner
  adoption does not call it.

All final profiles record unchanged tested head and clean source before/after,
commands, toolchain/configuration, submodules, complete logs and binary hashes.
No tests were skipped. All per-file manifests and original binary hashes verify.
Context retains before/after native binaries, development failures/corrections,
layout/table probes, source-tree equivalence and completed parent job metadata.
Known post-PASS offline-cache lock warnings remain; no retained store/cache was
cleaned. Final handoff changes eight Markdown files and one historical test
comment; all executable source is unchanged from the tested pin.

Raw final directories:
`/tmp/hcp-exchange-shared-faa7fc8e-{quick,focused,gcc,clang-values,gcc-values,layout,table,context}`
and `/tmp/hcp-exchange-direction-before`. Complete archive excluding build trees,
including four retained native executables:
`/tmp/hcp-exchange-shared-faa7fc8e.tar.gz`, SHA256
`705fc7436271a84ddc176e3dbc5f04d36df840128e2db26ec6e4e8731dff7742`.
These are contributor-local executions; hashes do not make raw evidence remotely
available or independently inspected.

Counters cover current-thread C++ new/new[] requests, not arbitrary malloc,
allocator rounding, RSS, whole-process peak, native/device/driver memory or
exception-runtime handling. Borrowed execution and caller memory remain separate.
Full local regression/compiler/IR/database matrices, GPU, browser and native
Windows were not run for this slice. This does not complete combined B/admission,
indexed guards, catalogue/mixed revisions, public producer adoption or #324.

#361 native [44124840860](https://github.com/Human-Cognome-Project/human-cognome-project/actions/runs/44124840860)
now reports success in all 12 jobs and its smoke passed. Its source acceptance
stands; it remains unmerged at inspection, with main still `3fd1936e`.
No duplicate workflow was dispatched. Review this implementation and reconcile
its own authoritative hosted CI and dependency/integration gates before merging.
