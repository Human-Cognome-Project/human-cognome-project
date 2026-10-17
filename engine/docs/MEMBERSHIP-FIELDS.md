# Contextual membership fields — Stage C

Implemented for review under [#144](https://github.com/Human-Cognome-Project/human-cognome-project/issues/144),
following the [accepted contract](../../docs/warm-toolbox-implementation-plan.md).
Stage B is accepted and merged in #143 at `6ccd5a3`. The full database-to-hot
adapter (D) remains queued in #139.

`assembly/membership_fields.{h,cpp}` consumes the accepted `LetterProjection`
and its immutable pinned recipe. The `membership_fields` CMake target depends
only on the pure projection/value targets, with no libpq or Taichi dependency.
It follows actual upward `member_of` references, verifies pinned endpoints and
rejects cycles. Reached target groups are deduplicated per owning letter, then
emitted once per target and active occurrence. No ancestry is inferred and no
source memberships are changed. The existing A/B recipe validation also rejects
missing or inconsistent references, cycles and duplicate direct source rows;
multiple valid upward paths remain supported.

For aggregate exposure, each reached field receives the letter's whole mass.
For byte exposure, each active byte occurrence inherits its **owning letter's**
reached groups and contributes its own mass. The byte definition's unrelated
classification memberships or another use's language memberships are not
substituted. Repeated byte positions and repeated uses remain distinct through
the accepted study/entry/path keys. Inactive alternatives emit no membership
edges. Exact aggregate/byte membership mass is conserved; equality of their
forces or trajectories is not asserted.

`MembershipContribution` retains `OccurrenceKey`, `FieldKey` with
`GroupKind::Membership`, the pinned target `ToolRef` and exact whole-mass count.
`MembershipFields::bind(projection, nodes)` extends that same study's bounded
`StudyInput` with explicit `(occurrence, group)` requests. Native opaque names
encode full keys losslessly. Binding a different study is rejected, even when
counts and definitions agree. Layout ownership and B's normalized scale bindings
remain unchanged.

Native `assembly::prepare` validates nonempty field names and active occurrence
references, deduplicates identical membership requests, and appends share-1
participation using the occurrence definition's exact mass. Field identity is
`(kind, value)`, so membership, parent-value and sibling fields remain distinct
even when their object keys match. Parent edges keep every original ordinal and
contribution. Group endpoint/revision/graph validity is checked in the pinned
value layer; the generic physics input does not resolve database addresses.

Group naming anchors remain metadata: NULL naming mass requires no replacement,
geometry, definition or particle slot. Native field arrays hold their inclusive
centroids, derived from live participating occurrences. The existing field
kernel, packed arrays and publication/seeding path are reused without a new law
or automatic group-particle creation. No database writes/schema changes, analyst,
scheduler, general recomposition or warm pin/materialization owner are added.

## Executed acceptance

[Acceptance record](../tests/membership_fields/acceptance.json), 2026-10-16:

- **25 pure C++ checks** cover direct/alphabet-mediated English deduplication,
  separate alphabet/language fields, two uses of A, shared English/French byte
  definitions without contextual leakage, full keys/revisions/counts, NULL naming
  anchors, unchanged source graphs, exact multibyte/Dutch/repeated-part membership
  masses and rejection of missing endpoints, inconsistent revisions and cycles.
- **44 native checks** cover the bounded input handoff, distinct field kinds,
  unchanged parent/sibling participation, repeated ordinals, duplicate request
  handling, invalid/inactive references and study mismatch. CPU fixtures exercise
  letter-only, byte-only and mixed exposure through `FixedStudy`: seeded centroid
  data, exact contextual field masses, the existing edge arrays, the first force
  and the isolated force added by memberships agree with independent inclusive
  centroid calculations. Inactive alternatives remain inert; shared French
  bytes/characters receive no English membership response.
- Existing `field_test`, `preparation_test`, `assembly_test`,
  `letter_projection_test` and `letter_projection_native_test` pass.

All sources are bounded fixtures; no database was mutated. Actual CPU execution
is reported above. GPU execution and full warm-to-hot integration are unexecuted
here and remain separate from this acceptance.

Pure reproduction needs C++17 and CMake only; the standalone project now runs
both projection and membership value suites:

```sh
cmake -S engine/tests/letter_projection -B /tmp/hcp-membership-values
cmake --build /tmp/hcp-membership-values -j 2
ctest --test-dir /tmp/hcp-membership-values --output-on-failure
```

With the existing native CPU engine build configured:

```sh
cmake --build engine/build --target membership_fields_test membership_fields_native_test -j 2
ctest --test-dir engine/build --output-on-failure -R '^membership_fields(_native)?_test$'
```
