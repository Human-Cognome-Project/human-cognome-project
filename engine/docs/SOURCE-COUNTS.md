# Exact source counts in prepared execution

Status: **Segment A implementation for review, 2026-10-24**, under the
[#183 accepted accounting plan](../../docs/weighted-centroid-accounting-plan.md)
and #177/#172. Baseline: `f8c4203dc67376479d5a7018d870f9906406fc69`.
Segment B's weighted candidate arithmetic remains pending.

## Source contract

[UpdateCommand](../src/field/prepared_update.h) now accepts optional uint64
`exact_mass` on `DefineParticle`, and `exact_count` on
`InsertParticipation`/`ReplaceParticipation`. These are trusted source metadata
from the existing prepared recipe/study, not query-supplied weights or numerical
starts. [Assembly](../src/assembly/study.h) already retains these integers.
No database lookup, production admission adapter or FixedStudy replacement is
introduced. New numerical starts remain private test fixtures.

An omitted value means unavailable. An explicitly supplied zero rejects, as
does metadata on an inappropriate operation. A supplied count requires a known
positive structural denominator for that occurrence, must not exceed it, and
must agree with the row's legacy f32 share under the existing source conversion:

```cpp
float(double(exact_count) / double(exact_mass))
```

This is a provenance consistency check. It does not replace the stored share
or change the physics to use a wider ratio. All positive uint64 count/denominator
ratios in range are above the f32 subnormal range; no global FP mode is changed.
Source integer values remain exact even when the double conversion or ratio
cannot represent them exactly. Native u64-to-f64 weighted arithmetic qualification
belongs to Segment B, not these host metadata checks.

The structural denominator is independent of current physical mass. The latter
remains in the existing resident f32 body data. Unknown legacy metadata stays
unknown, including share-one rows; the engine does not reconstruct integers from
the rounded share or infer provenance from numerical mass.

There is no global cap on counts across different fields. Several membership
fields may each contain an entire occurrence. Per-comparison coverage and
overlap validation remain Segment B work; this segment checks individual source
rows without silently normalizing a graph.

## Transaction, ownership and inspection

[PreparedStorage](../src/field/prepared_storage.h) owns committed and prospective
`source_masses` tables indexed by physical record and `source_counts` tables
indexed by ordered participation row. Zero encodes unavailable in these owned
uint64 tables; the command's optional field distinguishes omitted input from
an invalid supplied zero. The count tables belong to the corresponding full
handles/ledger and source generation. A row number alone is not enduring identity.

[The updater](../src/field/prepared_update.cpp) validates prospective endpoints,
denominators/counts and shares before native mutation. New provisional scratch
record names use the new definition's denominator. Publication associates it
with the actual native record returned by NodeManager, which need not have the
same index. Surviving denominators persist. Participation replacement supplies
the new count or explicitly clears its provenance by omission; it never silently
inherits the old count.

Counts follow their source rows through insertion/removal/compaction. Distinct
ordinals remain separate, even with equal endpoint handles. Retirement clears
the committed denominator; unused participation-count tails are zeroed. Recycled
records acquire only the new definition's metadata. No command mutates a
survivor's denominator through an unrelated participation edit.

A count-only change advances the existing topology/source generation. This
invalidates old geometry/cascade observations and requires explicit cascade
rebind, including when the edited row belongs to another field. Identical
metadata is `NoChange`. The existing physical contribution-sequence check still
uses occurrence/share, so changing only count provenance does not reseed field
history or introduce wakes. Source integer edits are not numerical movement.

Ordinary invalid input, stale rows and resource rejection leave committed source
metadata, generation, physical/control/activity/history state and Demand intact.
Prospective scratch may change. Unexpected backend failure follows the existing
unready policy. This adds no restore bank or recovery protocol.

The existing synchronized, const `prepared_storage()` observation exposes these
owned source tables for trusted native components/tests. Use the logical ledger
prefix and present record headers; retaining a row index across edits is invalid.
Query inputs cannot install or override source counts.

## Actual resources

Metadata is bounded by existing P/E/B reservations, with physical sentinels
`p=max(P,1)`, `e=max(E,1)` and `b=max(B,1)`. There is no payload per potential V
address and no additional identity-reference claim.

| Added/changed host storage | Payload |
|---|---:|
| Committed/prospective structural denominators | 2 × p × sizeof(uint64) |
| Committed/prospective ordered source counts | 2 × e × sizeof(uint64) |
| Command optional mass/count inputs | Included in actual sizeof(UpdateCommand) × b |

On the tested Clang/LLVM 15 ABI, source integers occupy eight bytes and command
size grows from 120 to 152 bytes; each command optional occupies 16 bytes. The
scratch increment is **16(p+e) +32b bytes** on this ABI. Relationship/contact
headers remain 64 bytes; contacts acquire no count table. The manifest uses
actual `sizeof` values rather than assuming those ABI measurements universally.

This segment adds no backend numerical arrays, kernel parameters/bindings,
native layouts, native counter widths or count transfers. Scratch preflight
charges both source copies and the larger commands before layout construction.
The updater copies p denominators and clears e prospective count entries per
batch, then validates/copies selected ledger rows. Existing structural-visit
counters are logical comparisons, not a count of every host copy/memory touch.
These are update-boundary costs, not per-tick scans or a scheduler.
Count-only changes currently use the normal prepared candidate/commit/index
path, including its existing transfers and synchronizations. The larger profile's
observed count-only update took 7.9–9.8 ms; the smaller profile took 57–60 us.
This implementation preserves the transaction and physical state; it does not
claim a single-store optimized metadata update.

## CPU evidence and remaining work

[Retained evidence](../tests/source_counts/README.md) pins source/backend/compiler,
profiles, raw results, resources and counter snapshots. Four modes cover both
CPU source host paths at P/G/E capacities 70/9/348 and 4096/257/20478, filling both
particle and participation banks, including the final records. The test adapter
prepares real definitions with thirds, repeated parent ordinals, sibling and
membership counts, and independent live copies.

All **66 native CPU suites passed in 401.74 seconds**. The new profiles reserve
132,396 scratch bytes at P/G/E=70/9/348 and 5,923,492 at 4096/257/20478, including
the optional one-node legacy cascade probe and the existing binding reservation.
Actual prepared binding payload is 9,304 bytes in either profile. Source metadata
adds no new bindings or native payload.

Tests preserve source values above 2^63, change counts without changing the
rounded share, clear/restore provenance, check stale/foreign/recycled references,
and reject zero, excess, missing-denominator, inconsistent-share and wrong-operation
metadata. Complete committed snapshots check ordinary and injected preclaim
rejection. Scratch S−1 rejects before layout mutation.

A companion execution receives identical legacy numerical inputs without exact
metadata. Results are compared bit for bit by logical occurrence/field identity;
independent native maps may assign different physical record numbers. The checks
cover body/field values, motion/history, activity/presence, ordered source rows,
focal controls, anchor and universal mass through 32 real tick/recycling cycles.
Known/unknown provenance alternates during reuse; survivor metadata and source-row
compaction are checked separately. A count-only edit must preserve physical state
and Demand within its own execution.

Measured update calls during first loading/reuse and recurring tick/inspection
calls observe zero linked C++ new/new[] calls. Known native
backing/pool/cursor/layout/compiler counters stay stable
through first touch and reuse. These observations exclude direct malloc,
physical residency/faults and driver internals. Payload excludes fixed C++ object
metadata, compiler memory and pool alignment. Ordinary host vectors are not pinned.

```sh
cmake --build engine/build -j3
ctest --test-dir engine/build -R '^source_counts_' --output-on-failure -j2
ctest --test-dir engine/build --output-on-failure -j2
```

Use the [matched native build](PREPARED-EXECUTION.md#reproduction). No new GPU
execution or PostgreSQL acceptance is claimed. Existing candidate arithmetic
still requires whole/disjoint contributions. Segment B must follow this segment's
review/regression evidence in its own PR. Live precision/publication/response,
native ripple settling and #156/#170 operational acceptance remain separate;
#172/#177/#156/#145 remain open.
