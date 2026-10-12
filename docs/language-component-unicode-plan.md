# Language memberships over shared Unicode value trees

Status: **Plan accepted in PR #127, with Patrick's membership clarification,
2026-10-11.** The [reusable offline importer and read-only export validator](../tools/linguistics/README.md)
are now implemented, with reproducible source/manifest evidence. Persistent
memberships and general warm aggregation remain planned.

## Intended result

Shared character values retain one identity. A character used across several
languages is a member of each applicable language, and of the appropriate
alphabet or punctuation groupings. A language's component inventory is
expressed through those memberships. Referencing an existing character does
not require constructing another single-character token for that language.

Language subdivisions use the same membership axis: English variant groups
are members of English, and English is a member of its broader groupings.
The cache manager follows these memberships and the literal value trees to
assemble language-appropriate warm objects and applies them to byte
structures. Each grouping has its own centroid representation. Alphabet and
punctuation have distinct groupings and centroids. Conceptual grouping work
comes later; this plan does not define new analyst behavior.

The current root decisions are `00` for the lightweight common computational
communication core and `02` for one shared multilingual Linguistics base.
Reserve `02.FS.LR.00.*` for language-frame word-component organization. Shared
core endpoints remain at their existing addresses; membership does not move
them into this branch or allocate copies. Higher-frame code examples remain
provisional. Shared script inventories and lexical bases can be reused, with
language/region differences stored as deltas and references.
This work focuses on iconographic structure and word commonality. Ideographic
modeling follows the `concept_db` work; the first major lexical ingestion
still targets every available Kaikki language.

**Addressing supersession (Patrick confirmed, 2026-10-11):** the shared
`02` Linguistics base replaces the earlier proposed English `01` / common
`01.00` allocation. Do not implement that earlier language-per-database layout.
Remaining allocations follow this plan's reviewed write slice; this change
does not assign a new meaning to `01`.

## Membership and construction contract

**Membership aggregates values and groups.** Store both directions through
`members` / `member_of`. A subgroup joins a broader group by membership; it
does not become a literal constituent of that broader group. For example:

```text
Australian English MEMBER_OF English
UK English         MEMBER_OF English
Canadian English   MEMBER_OF English
English            MEMBER_OF Germanic
Germanic           MEMBER_OF Indo-European
```

These are illustrative named relationships, not new address allocations.
Common membership in English and the practical lexical fallback route are
separate: Australia's usual UK-first lookup preference does not require its
group to be nested under UK English. Base/delta storage remains as discussed.

The same shared `A` value can belong to English and French, with its applicable
alphabet memberships. The shared `…` value can belong to their punctuation
groupings. In each language context, alphabet and punctuation use distinct
groupings, each with its own centroid representation for warm aggregation.

**Parents describe literal construction.** Store ordered constituents through
`token_parent` / `token_child`:

```text
multibyte Unicode character endpoint -> ordered byte codes -> ordered nibbles
ASCII character endpoint = existing byte code -> ordered nibbles
source-defined multi-character form -> ordered Unicode character endpoints
```

ASCII character endpoints reuse the existing byte-code tokens. A multibyte
character is one existing endpoint even though several bytes construct it.
For example, the ellipsis `…` is `U+2026`, encoded as `E2 80 A6`: alphabet/
punctuation membership refers directly to that **one** character endpoint.
Constructed three-period ellipses are deferred to later engine discovery work.

A source-defined form containing multiple Unicode scalars retains one ordered
parent per scalar. Keep repeated occurrences, exact scalar order and exact
UTF-8 reconstruction. Preserve source sequences rather than normalizing,
folding case or replacing them with visually similar forms. Case-related
forms require source evidence and language-appropriate casing rules.

A source-defined multi-character construction can itself have several language
memberships. Reuse its complete literal definition when applicable, preserving
its actual ordered parents. Single-character memberships need no such new
construction. Source inventory roles remain evidence for applicable groups;
an index exemplar is not automatically an alphabet member.

The active language's memberships and base/delta references determine which
values are assembled. The grouping centroid is a calculated/cache aggregation
representation, separate from a literal's structural mass. Do not replace a
character's `token.mass` with a group aggregate or put group containment into
`token_parent`. Concrete centroid preparation and nested rollup mechanics
remain cache-manager implementation work. Final grouping identities and any
new literal slot allocations belong to the reviewed write slice.

Cold definitions supply the native records described in
[first-stage-data-object-model.md](first-stage-data-object-model.md): resolved
identities, ordered parent occurrences and exact structural counts for literal
construction. Language/group memberships are separate from those parent
occurrences; the bounded API does not implement group centroid aggregation.
Warm live uses remain separate from reusable definitions. Endpoint-to-character
use is the previously identified 1:1 equivalence; storage/composition depth does not
create an extra conceptual LoD step. General cache-manager assembly remains
planned; the bounded native preparation API is already built.

## Machine-readable sources

Use a scripted import with pinned inputs and source hashes:

- [Unicode Character Database 17.0.0](https://www.unicode.org/Public/17.0.0/ucd/):
  scalar identity, names, general categories, script/script-extension metadata
  and casing data. UTF-8 is computed from the exact scalar sequence.
- [CLDR 48.2](https://github.com/unicode-org/cldr/tree/11299982335beb974c1c63c45265184e759c0f41/common/main),
  commit `11299982335beb974c1c63c45265184e759c0f41`: language/locale exemplar
  sets, including main, auxiliary and punctuation inventories. Retain each
  set's source role and draft status. Index exemplars are additional evidence,
  not a declaration of a language's alphabet or alphabet order.
- The complete selected Kaikki exports, when lexical ingestion begins:
  source language codes and observed written forms extend the coverage audit.
  Observed characters and declared alphabet inventories remain distinguishable.

Parse CLDR's UnicodeSet syntax with ICU, including ranges, escapes and
multi-character strings. Resolve CLDR aliases, explicit parent locales and
inheritance markers before producing a full inventory. Case expansion must
respect the language, including Turkish dotted/dotless I. Do not treat a
Unicode script block as the alphabet of every language using that script.

CLDR supplies many language inventories, but does not cover every Kaikki
language or every historical orthography. Produce a coverage matrix keyed by
source language/locale/script, with provenance, inherited coverage and missing
inventory sources. Additional machine-readable sources can be added without
changing the parent-resolution contract. An absent inventory must stay visible
in the report; it cannot silently remove a language from the ingestion scope.

The existing `sources/unicode_table.txt` is an approximately 2,000-character
summary with explicitly incomplete coverage, so it cannot drive this import.

## Script output and acceptance checks

The first implementation is an offline bootstrap/import script. It emits a
deterministic candidate manifest and a coverage report before writing data.
Python here is restricted to bulk bootstrap population and preparation/checks
of those batches. Offline operation alone does not authorize Python components.

The candidate manifest separates reusable values, group descriptions and
membership edges. Each source record retains its language/locale, script
context where available, inventory role, exact scalar sequence and pinned
provenance. A single-scalar value references its existing canonical core
identity and store context. A genuinely new multi-character literal carries
ordered endpoint parents. Group descriptions distinguish languages, their
subgroups, alphabet groupings and punctuation groupings, including the required
centroid representation without inventing centroid coordinates.

Deduplicate memberships by exact member/group identity, retaining their source
evidence. Preserve literal repetitions at separate parent ordinals. Report
source memberships, unique shared values, actual group memberships and any
case-expanded candidates as separate counts. UTF-8 bytes and derived structural
counts remain validation evidence. Source keys are preview identifiers;
final group and new-literal addresses have not been allocated.

Before a batch is accepted, verify automatically:

1. Each sequence contains valid scalars and reconstructs the exact input bytes.
2. Every referenced endpoint exists in the target core inventory with complete
   structural mass and the expected construction: ordered nibble parents for
   ASCII byte-token endpoints, ordered byte parents for multibyte endpoints.
   Missing or inconsistent endpoints produce a report rather than a core edit.
3. Ordinals are contiguous and repeated parents remain separate occurrences.
4. Language, language-subgroup, alphabet and punctuation memberships use
   `members` / `member_of`; both directions agree with their owning records.
   Every grouping has its distinct centroid representation. Group containment
   creates no literal parent edges or duplicate character definitions.
5. A rerun matches the existing full construction, producing no duplicate
   records; conflicting definitions are reported before writing.
6. Per-language coverage and expected batch totals agree with the pinned input.

## Completed read-only preview

An offline preview parsed five inventory roles for six base locales using
CLDR 48.2 and Unicode 17.0.0. These counts are **source memberships**, not
alphabet-letter counts or allocated HCP objects:

| Locale | Source memberships | Distinct scalar sequences | Memberships using multiple endpoints |
|---|---:|---:|---:|
| English `en` | 122 | 122 | 0 |
| Czech `cs` | 133 | 133 | 2 |
| Turkish `tr` | 132 | 131 | 0 |
| Arabic `ar` | 111 | 83 | 0 |
| Persian `fa` | 119 | 85 | 0 |
| Urdu `ur` | 154 | 107 | 0 |

The 771 CLDR source memberships reference 247 distinct character endpoints.
They are evidence rows, not 771 new character objects or the final HCP edge
count; the membership-driven importer must derive shared-value/group edges.
The preview preserves inventory roles, expands literal UnicodeSet patterns with ICU 76,
and resolves the selected base locales' empty auxiliary-punctuation fields
through `root`. It does not implement general locale inheritance, aliases or
language-sensitive case expansion, and is not the production importer. Full
property-based parsing needs an ICU version compatible with the pinned UCD.

| Source form | Scalar sequence | Derived core value reference(s), in order | UTF-8 |
|---|---|---|---|
| English `A` | `U+0041` | `00.00.00.00.23` | `41` |
| English `a` | `U+0061` | `00.00.00.00.2Z` | `61` |
| English `.` | `U+002E` | `00.00.00.00.1k` | `2E` |
| English `…` | `U+2026` | `00.00.00.04.bi` | `E2 80 A6` |
| Czech `ch` | `U+0063 U+0068` | `00.00.00.00.2b`, `00.00.00.00.2g` | `63 68` |
| Turkish `ı` | `U+0131` | `00.00.00.01.2r` | `C4 B1` |
| Turkish `İ` | `U+0130` | `00.00.00.01.2q` | `C4 B0` |
| Arabic `؟` | `U+061F` | `00.00.00.01.ND` | `D8 9F` |

The preview checked all 256 byte definitions against the retained
[2026-09-28 encoding-floor export](../data/postgres/snapshots/2026-09-28-encoding-floor/MANIFEST.md):
addresses, mass-2 byte values and ordered mass-1 nibble constituents. The
export's parent-row mass cells remain NULL; this check does not populate them.
Non-ASCII addresses are derived from the native address layout in
[PR #111](https://github.com/Human-Cognome-Project/human-cognome-project/pull/111),
at head `5120ee6e6316bf76b6cd7a91ec51913054464877`.

A separate pure-data check compared the preview's address calculation and
UTF-8 encoding to that native bootstrap implementation for **all 1,112,064
valid Unicode scalars**, including ASCII. Addresses were strictly increasing;
address and UTF-8 mismatches were zero. This verifies derivation, not the
existence/integrity of every row in a current live core store. No database was
connected, created, restored or populated for either check.

## Phased execution

**Offline slice built:** `tools/linguistics/` pins CLDR 48.2, UCD 17.0.0 and
ICU4J 78.1, resolves source inheritance/aliases and emits shared values,
symbolic group descriptions, deduplicated reciprocal membership candidates
and coverage. Its English pilot references 84 existing values. The expanded
six-locale run preserves the 771 source memberships while separating casing
candidates and actual group edges. Index exemplars remain evidence only;
multi-scalar case mappings do not fabricate literal constructions. The
[checked-in evidence](../data/linguistics/previews) is regenerated and compared
in CI. The validator inspects supplied plain/gzip COPY exports without SQL
execution or restoration. The current **2026-10-03 full UTF-8 export**, extracted
from #111 into [snapshot PR #130](https://github.com/Human-Cognome-Project/human-cognome-project/pull/130),
passes the English and six-locale endpoint checks with zero missing values or
observed construction conflicts. The September 28 floor remains historical
evidence and is correctly reported as partial; its 14 missing English
punctuation endpoints are present in the current export. Current reports
validate 286 and 492 endpoints/dependencies respectively, using the supplied
export without connecting to a live core. No persistent membership or centroid
assembly is implemented by this offline slice.

**Multilingual bulk seed preparation, 2026-10-12:** the initial
profile now covers 20 languages across Latin, Greek, Cyrillic, Hebrew, Arabic,
Armenian and Georgian scripts. It has 913 applicable iconographic values,
65 group descriptions and 5,460 membership edges, with no source gaps. All
1,082 referenced core endpoints/dependencies pass the current export check.
The batch retains shared identities, exact forms and reciprocal memberships
for database seed review. A flat catalog lists the complete batch's alphabet
and punctuation memberships. Persistent population and unchanged-rerun checks
remain to be completed. The generic Python selector and its working-area
bundle format were removed; this work is limited to database seeding.
The current test objects are individual letters in each language.

The objective is functional bootstrapping: create an initial connected
framework around which further structure can form. UTF-8 is the encoding
target for now. Once sufficient density supports viable physics, subsequent
material and other encoding-table qualification come through standard methods.
This stage does not assign a new analyst architecture or impose another
general physics qualification gate.

1. **Script and manifest:** implement the reusable source importer and
   read-only target-core validator; generate per-language inventories and
   report missing sources. Begin with English's 52 ASCII letters and 32
   CLDR punctuation forms, including the single-character ellipsis. These
   are 84 referenced existing values to assign applicable language/alphabet/
   punctuation memberships, not 84 new single-character definitions. Emit
   the language/alphabet/punctuation grouping descriptions and English's
   broader-group membership evidence. Then add auxiliary inventories/locales.
2. **Persistent link slice:** finalize grouping/new-literal allocation and the
   bounded cross-store reference/write contract, then add the reviewed
   memberships and any required group naming constructions through the owning
   controller. Validate shared identities, membership reciprocals, distinct
   group centroid representations and an unchanged rerun. Only genuinely new
   literal forms require new character-endpoint parent lists.
3. **Warm assembly:** feed the pilot memberships and value trees to the
   cache-manager/native preparation boundary. Check that an English selection
   assembles the applicable shared character objects and distinct alphabet/
   punctuation group centroid representations over the exact byte structures.
   Follow English variant -> English -> broader-group membership links. This is
   data/assembly validation, without reopening general physics qualification.
4. **Expand coverage:** repeat the source-driven cycle for additional alphabetic
   language inventories and local deltas. Retain source gaps for later inputs.
   Detailed conceptual and ideographic modeling follows its own discussion.

The current controller resolves references in one PostgreSQL connection, and
the schema's parent/member foreign keys target its local `token` table.
Those keys cannot directly reference a separate Linguistics database. The
generated manifest can carry core-value references and intended membership
edges now. Cross-store membership reciprocals need the owning-store/write/
retry contract even when no new single-character literal is required. The
native cross-store boundary must be agreed and implemented before persistent links.
Core inventories are reused rather than repopulated as a side effect.
Previously identified record-integrity work in PR #111 remains separate and
is not cleared by the derivation check. Appropriate source/data/native checks
belong in local CI.
