# Native letter/byte projection — Stage B

The [parent-target selector implementation](PROJECTION-PARENT-TARGET-INDICES.md),
tested source `c63bf01d`, implements #397 review 7052622147. Private parent rows
retain full numeric source and independent occurrence/parent selectors, with
kind and ordinal/count preserved. Steady readers validate role; preparation
handles the legitimate intermediate combined pool separately. Active owning
exports remain complete. Actual width/storage/work/output costs and source-pinned
CPU qualification are recorded separately; runtime review and hosted CI remain.

The [parent field-target inspection](PROJECTION-PARENT-TARGETS.md), tested source
`f551d14f`, follows #396's next-inspection direction. It attributes full field
scope/address backing in active/spare pools, checks the relation to the existing
recipe and measures independent source/parent selectors. Candidate ABI and guard
work are separate from actual storage; production and saving are unchanged
at that source. #397 accepts the bounded implementation direction above.

The [parent study implementation](PROJECTION-PARENT-HEADER.md), tested source
`22482caa`, implements #395 review 7050686178. Active/spare private parent rows
retain numeric sources under the existing projection header. Public `parent_count()`
and `parent(index)` replace the vector reference with complete independently
owning active values. Field kind/object, ordinal/count, row order and reserved
pool reuse remain. Actual storage/work/output costs and CPU qualification are
recorded separately. #396 provisionally accepts final `2fc9d157`; smoke and
values passed, with remaining native qualification and maintainer integration separate.

The [parent study ownership inspection](PROJECTION-PARENT-STUDY-IDENTITY.md),
tested source `a9011a88`, follows #394's accepted next-inspection direction.
Active/spare parent rows each retain a complete study UUID despite the existing
owned header. Actual backing, separate row ABI, complete copy costs, contextual
refusal/lifetime and both-view publication are observed without a runtime change
at that source. #395 accepts its bounded implementation direction above.

The [checked byte-owner implementation](PROJECTION-BYTE-OWNER-INDICES.md), tested
source `ec5ca687`, implements #393 review 7047998750. Private byte provenance
retains an independent aggregate-owner index; reads validate byte/owner recipe
sources and full character/ordinal association in the owning projection.
Construction-time invariants avoid frozen recipe rescans. Full public provenance,
parent keys and native execution authority remain. #394 accepts final `1f71eb76`;
its native and smoke workflows passed and the maintainer integrated it at `66b7ef3a`.

The [projection-owned study implementation](PROJECTION-HEADER-IDENTITY.md), tested
source `68062bde`, implements #391 review 7045231362. Private rows keep numeric
entry/full path under their existing owned header; contextual reads take the
owning projection and checked index. Public exports, byte-owner and parent keys
remain complete. Source/revision/node/mass and native claim/generation validation
retain their separate authorities. #392 passed hosted qualification and is
integrated at `8627c262`. The [accepted byte-owner key inspection](PROJECTION-BYTE-OWNER-KEYS.md)
records original attribution before the independent-index runtime above.

The [recipe-index implementation](PROJECTION-DEFINITION-INDICES.md), source
`223e874a`, implements #389's accepted ownership/output contract. Private rows
retain checked definition/source indices in their projection's existing exact
immutable recipe. Public `occurrence_count()` / `occurrence(index)` export complete
owning `ProjectedOccurrence` values; the old `occurrences()` vector-reference API
is removed. Internal/frozen readers use checked contextual reads, and the viewport
producer uses explicit owning exports. Byte/complete-character provenance, ordinary
and selected copy independence, private-spare sharing and native authority remain.

The supplied-state path below is retained as the **legacy v1 fixture**. The
[#170 structural segment](../../kernels/database/warm/STRUCTURAL-RECIPES.md) adds
`normalize_structural_letters` and `bind_structure` for v2: all alternatives have
structural identities, intrinsic geometry and explicit focus, with no world
starts or numerical slots. It preserves the same exact UTF-8 construction and
membership accounting. Native engine-owned admission and enduring workbench
integration remain later segments; v2 cannot call the legacy numerical `bind`.

Accepted and merged in [#143](https://github.com/Human-Cognome-Project/human-cognome-project/pull/143)
at `6ccd5a3`, closing #142,
following the [accepted contract](../../docs/warm-toolbox-implementation-plan.md).
Stage A is accepted and merged in #141 at `e7acdea`. C in #146 is reviewed with
all nine hosted checks green, accepted and merged at `43f418a`. D requires the separate
[#145 pre-D workbench design review](../../docs/pre-d-hot-workbench-design.md)
and explicit #139 release. C acceptance alone does not release D.

`assembly/letter_projection.{h,cpp}` consumes Stage A's `warm::ToolRef`, ordered
`Parent`, geometry and normalized `Bundle` values. It has no PostgreSQL or Taichi
dependency. `study_input.h` contains the existing host input records, separated
from runtime includes without changing their meaning. `warm_values` is shared
by the storage and projection targets; neither depends on the other.

## Preparation boundary

1. Supply a `warm::Bundle` containing metadata, pinned definition closure,
   geometry and optional contacts. Nodes, occurrences and selections start empty.
2. Supply `LetterRequest`s with distinct uint64 entry IDs, an exact letter
   reference, exposure, aggregate initial state and every byte alternative's
   explicit `(character ordinal, byte ordinal)` placement/velocity. Geometry and
   placements are caller inputs; projection does not infer packing.
3. `normalize_letters` resolves recorded character/byte construction, checks
   exact source-form UTF-8 and fills the same normalized recipe Stage A validates
   and persists. It preserves original tool bodies, NULL parent-row masses,
   revisions and metadata. It allocates no object addresses or publication IDs.
4. `project_letters(recipe)` validates those values and creates a new study UUID,
   typed occurrence/provenance records and active ordered parent contributions.
   Slots cover **all** alternatives, sorted by entry/path. Full scope/address
   and study/entry/path keys encode losslessly into native opaque strings.
5. `projection.bind(nodes)` supplies bounded `StudyInput` values for the existing
   native `prepare`. The caller supplies and owns the real SNodes matching the
   normalized recipe: Root, optionally its direct primary letter child. Native
   preparation verifies the actual tree and performs numerical conversion.

Every projection has a new study identity; this API does not refresh a live
study. Changing exposure requires a new normalized study/layout even with equal
counts. Creating layouts, acquiring warm pins, staging a `FixedStudy`, device
transfer and readiness belong to D. The small `bind` seam constructs values
only; it does not perform that full adapter's work.

## Preserved construction and scale

Aggregate definitions retain source-character parents. Bytes retain their two
ordered mass-1 nibble parents. Nibbles and equivalent core characters do not
become additional particles. An ASCII character already is a byte endpoint;
its nibble parents are never interpreted as bytes. For non-ASCII characters,
ordered byte parents must agree with the source's uppercase hexadecimal UTF-8
notation. Each character must encode exactly one valid scalar; concatenation
must equal the linguistic source form byte for byte, without normalization or
casefolding. Byte and nibble values and exact masses are checked as well.

`ProjectedOccurrence` retains the pinned definition, active flag, slot, node and
byte owner/character reference/ordinals. Complete-character equivalence is
recorded only for a single-scalar whole. `ParentContribution` retains the live
occurrence, typed parent-value field, source ordinal and exact contribution;
repeated parents remain distinct edges to the same field. Membership graphs are
preserved as source values but generate no membership fields in B. Stage C
separately derives contextual membership fields from these same pinned values.

| Profile | Root units | Letter binding |
|---|---|---|
| Letter-only | 1; dormant byte alternatives use an inactive placeholder | Root, 1 |
| Byte-only | Active bytes, 1 | Inactive aggregate, direct primary child, 64 |
| Mixed | Active bytes, 1 | All aggregates, direct primary child, 64 |

Naming/compression attachments in native preparation inherit their enclosing
units at 1:1. Geometry, position and velocity remain local inputs until native
preparation. Position 2 / radius 0.5 becomes 2 / 0.5 for a letter-only study,
128 / 32 for a mixed active letter and 2 / 0.5 for a mixed active byte. Mass is
unchanged. No cross-exposure trajectory equivalence is claimed.

## Executed checks

[Acceptance record](../tests/letter_projection/acceptance.json), 2026-10-16:

- 53 standalone pure C++ checks: English/French A, 2/3/4-byte scalars, Dutch
  `íj́` (4+2+4, bytes C3 AD 6A CC 81), repeated `aá`, two uses of A, exact
  source values, typed paths and normalized recipes. Negative cases cover
  malformed/missing/repeated/reordered parts, inconsistent revisions, unknown
  or overflowing mass, missing geometry, bad UTF-8/nibbles and exact spelling.
- 38 checks through the existing native `StudyInput`/`prepare`: all exposure
  profiles, geometry/state conversions, active contacts, exact mass and repeated
  parent contributions, full identities, distinct language sibling fields with a
  shared parent field, f32 overflow and 1:1 naming/compression inheritance.
- Existing `preparation_test` and CPU `assembly_test` pass. The unchanged Stage A
  storage/acceptance client also builds against the extracted value target.

These are pure fixtures and bounded native preparation checks. No database
writes, full database-to-hot materialization or GPU execution are asserted.

Standalone reproduction needs a C++17 compiler and CMake only:

```sh
cmake -S engine/tests/letter_projection -B /tmp/hcp-letter-projection
cmake --build /tmp/hcp-letter-projection -j 2
ctest --test-dir /tmp/hcp-letter-projection --output-on-failure
```

With the existing native engine build configured:

```sh
cmake --build engine/build --target letter_projection_test letter_projection_native_test preparation_test assembly_test -j 2
ctest --test-dir engine/build --output-on-failure -R '^(letter_projection_test|letter_projection_native_test|preparation_test|assembly_test)$'
```
