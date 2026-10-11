# Language component trees over core Unicode endpoints

Status: **ACCEPTED by maintainer review in PR #127, 2026-10-11.** Source research and a read-only
mapping preview have been completed. This PR changes documentation only;
database creation, population and runtime changes follow approved slices.

## Intended result

Every language's letters and punctuation forms have construction definitions
whose ordered parents are the exact character endpoints in `hcp_core`.
The cache manager follows these value trees to assemble language-appropriate
warm objects and applies them to byte structures. In an English context it
assembles English alphabet/punctuation objects; later conceptual contexts use
their corresponding conceptual objects. This proposal supplies the linguistic
construction data, without defining new analyst behavior.

The current root decisions are `00` for the lightweight common computational
communication core and `02` for one shared multilingual Linguistics base.
Reserve `02.FS.LR.00.*` for language-frame word components. Higher-frame code
examples remain provisional. Shared script inventories and lexical bases can
be reused, with language/region differences stored as deltas and references.
This work focuses on iconographic structure and word commonality. Ideographic
modeling follows the `concept_db` work; the first major lexical ingestion
still targets every available Kaikki language.

**Addressing supersession (Patrick confirmed, 2026-10-11):** the shared
`02` Linguistics base replaces the earlier proposed English `01` / common
`01.00` allocation. Do not implement that earlier language-per-database layout.
Remaining allocations follow this plan's reviewed write slice; this change
does not assign a new meaning to `01`.

## Construction contract

```text
language letter or punctuation definition
  -> ordered hcp_core Unicode character endpoint(s)
     -> ordered UTF-8 byte-code parents
        -> ordered nibble parents
```

ASCII character endpoints reuse the existing byte-code tokens. A multi-byte
character endpoint is one parent of the language form, regardless of how many
bytes construct that endpoint. For example, the ellipsis `…` is `U+2026`,
encoded as `E2 80 A6`: its language record has **one** character parent.
Constructed three-period ellipses are deferred to later engine discovery work.

A source-defined form containing multiple Unicode scalars retains one ordered
parent per scalar. Keep repeated occurrences, exact scalar order and exact
UTF-8 reconstruction. Preserve source sequences rather than normalizing,
folding case or replacing them with visually similar forms. Case-related
forms require source evidence and language-appropriate casing rules.

Language alphabet membership and component construction are separate data:
membership chooses the applicable inventory; parents define the literal
assembly. Shared core characters retain their identities. The active language
and its base/delta references determine which language values are assembled.
Final grouping locations and component slot allocations belong to the write
slice, after the generated inventory can be reviewed.

Cold definitions supply the native records described in
[first-stage-data-object-model.md](first-stage-data-object-model.md): resolved
identities, ordered parent occurrences and exact structural counts. Warm live
uses remain separate from reusable definitions. Endpoint-to-character use is
the previously identified 1:1 equivalence; storage/composition depth does not
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
Python is appropriate for offline I/O under `AGENTS.md`; database/kernel and
warm-cache runtime changes continue through the native C++ boundaries.

Each candidate carries its source language/locale, script context where
available, inventory role, literal scalar sequence, source/version/hash,
language-frame destination and ordered parent references. Parent references
retain the canonical address and owning-store context. UTF-8 bytes and derived
structural counts are validation evidence. Preview candidates use source keys;
their final `02` component identities are not allocated yet.

Before a batch is accepted, verify automatically:

1. Each sequence contains valid scalars and reconstructs the exact input bytes.
2. Every referenced endpoint exists in the target core inventory with the
   expected ordered byte parents and complete structural mass. Missing or
   inconsistent endpoints produce a report rather than an implicit core edit.
3. Ordinals are contiguous and repeated parents remain separate occurrences.
4. Memberships and cross-store reverse links agree with their owning records.
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

The 771 memberships reference 247 distinct character endpoints. The preview
preserves inventory roles, expands literal UnicodeSet patterns with ICU 76,
and resolves the selected base locales' empty auxiliary-punctuation fields
through `root`. It does not implement general locale inheritance, aliases or
language-sensitive case expansion, and is not the production importer. Full
property-based parsing needs an ICU version compatible with the pinned UCD.

| Source form | Scalar parents | Derived core endpoint(s), in order | UTF-8 |
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

1. **Script and manifest:** implement the reusable source importer and
   read-only target-core validator; generate per-language inventories and
   report missing sources. Begin with English's 52 ASCII letters and 32
   CLDR punctuation forms, including the single-character ellipsis. This is
   an 84-form pilot, then add its auxiliary inventory and other locales.
2. **Persistent link slice:** finalize component allocation and the bounded
   cross-store reference/write contract, then insert the reviewed pilot
   additively through the owning controller. Verify the generated construction,
   reciprocal links, structural counts and an unchanged rerun.
3. **Warm assembly:** feed the pilot value trees to the cache-manager/native
   preparation boundary and check that an English selection assembles the
   corresponding English objects over the exact byte structures. This is
   data/assembly validation, without reopening general physics qualification.
4. **Expand coverage:** repeat the source-driven cycle for additional alphabetic
   language inventories and local deltas. Retain source gaps for later inputs.
   Detailed conceptual and ideographic modeling follows its own discussion.

The current controller resolves references in one PostgreSQL connection, and
the schema's parent/member foreign keys target its local `token` table.
Those keys cannot directly reference a separate Linguistics database. The
generated manifest can carry the intended core references now; the native
cross-store boundary must be agreed and implemented before persistent links.
Core inventories are reused rather than repopulated as a side effect.
Previously identified record-integrity work in PR #111 remains separate and
is not cleared by the derivation check. Appropriate source/data/native checks
belong in local CI.
