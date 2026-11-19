# Contextual membership fields — Stage C

[Catalogue-owned unique targets](MEMBERSHIP-TARGET-TABLE.md) at `6da22846`
implements #387's accepted table/index direction. Private rows retain checked
indices into independently owned complete pins, preserving participation order,
kind/count and detached public values. Net owner costs, live peak/work increases,
empty/single/repeated controls, copy/export overhead and qualified local CI are
recorded there; runtime review and authoritative GitHub CI remain pending.
The accepted [inspection](MEMBERSHIP-TARGET-PINS.md) retains its before evidence.

[Catalogue-owned study identity](MEMBERSHIP-HEADER-IDENTITY.md) at `e7733a8c`
implements #385's accepted direction from `fc0e7f65`. Private rows retain numeric
entry/path under one owned study header; public values keep complete owning
keys. Actual retained/peak reductions are 680 / 1,360 bytes in live/catalogue
factories and 303 / 1,212 / 606 in real owners. Explicit export and ordinary
catalogue copy costs are measured separately. Local CPU qualification passes;
runtime is integrated in #386 at main `0e3c0fdf`. The accepted
[inspection](MEMBERSHIP-STUDY-IDENTITY.md) retains its before evidence.

**Contributor implementation — 2026-11-17 (awaiting review):**
[target-pin authority](MEMBERSHIP-PIN-AUTHORITY.md) at `4f27d26d` implements
the duplicate field-object removal authorized by #357. Rows retain explicit
membership kind, full occurrence/target pin and count; consumers use the pin's
object directly. Actual collector measurements, census, preserved failures and
final Clang/GCC qualification are recorded there. Short group addresses and
the context/exposure/native semantics below remain. Review and authoritative
hosted CI are pending.

**Ownership comparison — 2026-11-16 (awaiting review):**
[field-key cost attribution](MEMBERSHIP-FIELD-KEY-COST.md) at `ec21b153` measures
the ObjectKey duplicated beside the full target ToolRef in each returned row.
Production and the accepted contract below remain unchanged. The bounded
proposal reuses the existing pin object while retaining explicit membership
kind, context/count/revision, shorter valid group addresses and detached lifetime.
Actual live/catalogue cost and all 3,808 membership value controls agree under
Clang/GCC; local native consumer and quick CI also passed. Removal awaits review.

Accepted and merged in #146 at `43f418a`, closing [#144](https://github.com/Human-Cognome-Project/human-cognome-project/issues/144),
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

Public `MembershipContribution` retains an owning `OccurrenceKey`, explicit
`GroupKind::Membership`, the complete target `ToolRef` and exact whole-mass count.
`contribution_count()` and `contribution(index)` replace the reference accessor;
each extracted value independently owns its full study/path/pins after catalogue
and selected-domain death. Export may allocate. Private rows hold only numeric
entry/path beside kind/checked target index/count, with one owning study header
and one target table per catalogue. Copies own independent table/pin/path data;
copy assignment stages a complete replacement in the destination domain, and
moves transfer the correlated vectors together. Internal consumers validate full
study/recipe associations and indices before use. The field object comes from
the resolved `target.object`; no second owning ObjectKey is retained beside the
pin. Bind readers reject nonmembership kind.
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
