# Offline linguistic inventory importer

**BUILT:** pinned Unicode/CLDR acquisition, ICU UnicodeSet parsing and locale
casing, source inheritance/aliases, shared-value candidate manifests, symbolic
groups, reciprocal memberships, coverage reports and a read-only COPY-export
validator. This is bootstrap/import I/O, outside the native runtime.

**PLANNED:** final group/new-literal addresses, persistent cross-store links,
owning-controller writes/retries and native group-centroid assembly. Group
centroid descriptions are requirements with pending state; no coordinates or
aggregate masses are calculated here. The manifest is a proposal, not a
write-ready population batch.

The purpose is functional bootstrapping: seed a connected representational
framework around which subsequent structure can form. UTF-8 is the encoding
target for this stage. Once sufficient density supports viable interaction,
further material and other encoding-table qualification follow the standard
methods. This offline work supplies definitions and memberships for that
handoff; native centroid/geometry/placement preparation remains separate.

The [accepted plan](../../docs/language-component-unicode-plan.md) defines the
membership/construction contract. Single scalars reference existing `hcp_core`
identities. Source-defined multi-scalar forms retain exact ordered endpoint
parents and unallocated shared-literal identities. Repetitions, case and
combining sequences are preserved, without normalization. Each alphabet and
punctuation grouping has its own pending centroid description. Language
hierarchy edges are explicit reviewed bootstrap metadata, separate from CLDR
source inheritance and lexical fallback preference. Concrete linguistic
addresses remain unallocated in the shared `02` scope.

## Requirements and pinned sources

Use Python 3.10+ and a JDK 21+ with its source-file launcher. The importer uses
the standard library and the pinned ICU4J 78.1 jar; it does not use the host's
Python/system Unicode tables. The native derivation audit additionally needs
`c++` with C++17 and GNU-compatible section garbage collection (Linux).
No PostgreSQL service or credentials are needed.

[sources.lock.json](sources.lock.json) pins Unicode 17.0.0 data, CLDR 48.2 at
commit `11299982335beb974c1c63c45265184e759c0f41`, and ICU4J 78.1 to exact
SHA256 hashes. ICU reports its own Unicode version before processing a batch;
version mismatches fail. Downloads use TLS verification and fail on hash
mismatch, including corrupt cached inputs. Downloads are approximately 81 MB
and live under ignored `sources/data/linguistics/`. Required public hosts are
`www.unicode.org`, `codeload.github.com` and `repo.maven.apache.org`; the optional
native audit also uses `raw.githubusercontent.com`.

Run from the repository root:

```bash
python tools/linguistics/import_inventory.py fetch
python tools/linguistics/import_inventory.py preview \
  --locales en --roles main punctuation \
  --out sources/data/linguistics/english-pilot
python tools/linguistics/import_inventory.py preview \
  --locales en cs tr ar fa ur \
  --out sources/data/linguistics/six-locales
python tools/linguistics/import_inventory.py preview \
  --profile tools/linguistics/bootstrap-locales.json \
  --out sources/data/linguistics/bootstrap-20
```

`--cache PATH` is a global option before the subcommand. Fetch once and subsequent
preview/validation work is offline. The validator requires Python only.
Select additional CLDR locale IDs through `--locales`; hyphens are accepted
and canonicalized to underscores. Unknown selected locales remain visible
as gaps. An absent/empty main inventory never becomes a complete alphabet
by borrowing root punctuation. CLDR coverage is not every Kaikki language
or historical orthography; eventual Kaikki ingestion still needs additional
machine-readable inventory sources and observed-form coverage.

## Membership-selected iconographic bundles

**BUILT offline:** select one or more supplied language, alphabet or punctuation
groups to export their iconographic values and construction expectations:

```bash
python tools/linguistics/import_inventory.py select \
  --manifest sources/data/linguistics/bootstrap-20/manifest.json \
  --groups language:en language:el language:he \
  --out sources/data/linguistics/bootstrap-20/selected-bundle.json
python tools/linguistics/import_inventory.py select \
  --manifest sources/data/linguistics/bootstrap-20/selected-bundle.json \
  --groups punctuation:en \
  --out sources/data/linguistics/bootstrap-20/punctuation-en.json
```

Selection follows membership downward into applicable groups and shared values.
A combined selection keeps one reusable definition for a shared value. Further
narrowing preserves boundary membership references to the larger view without
activating those outside groups. Each bundle retains exact visible forms,
scalar sequences, UTF-8 bytes, source references and separate group-centroid
requirements. It also carries closed construction expectations through the
character/byte/nibble layers with ordered positive structural counts. Those
dependencies do not become additional alphabet/punctuation members.

Any supplied locale inventory can use the same selector. The twenty-language
profile is a demonstration covering English, French, German, Spanish,
Portuguese, Italian, Dutch, Czech, Turkish, Greek, Russian, Ukrainian,
Bulgarian, Serbian, Hebrew, Arabic, Persian, Urdu, Armenian and Georgian.
Only groups populated by source inventories supply iconographic entries;
unknown selectors fail, and known unpopulated groups produce an explicit
empty-selection result (exit 2). Selection is a membership view, with lexical
fallback and regional delta rules retaining their separate scope.

The bundle supplies data for later population of a selected working area.
Native cache-manager assembly will provide group centroids, geometry, live
occurrence identities, slots and study-node placement. This exporter creates
no runtime fields, cold allocations or persistent records.

## Candidate output

`manifest.json` separates:

- Shared `values`: one canonical core reference per scalar; exact, unallocated
  multi-scalar constructions such as Czech `ch` have ordered core parents.
- `groups`: symbolic language/subgroup, alphabet and punctuation descriptions,
  plus reviewed broader groupings from `bootstrap-groups.json`. A group has
  its distinct pending centroid, without literal constituent parents.
- Deduplicated `memberships` and matching one-level `members` / `member_of`
  lists. Edge evidence retains source roles and case origins. Local kind
  labels describe the preview only; they do not add a `token.type` column.
- `inventories`, `source_forms` and `case_candidates`: exact patterns, resolved
  owner/path, draft status, source hash and alias/parent trace. Script metadata
  comes from UCD Scripts/ScriptExtensions; source identity context is retained
  where CLDR declares it.

Main and auxiliary forms supply alphabet-component memberships; punctuation
and auxiliary punctuation supply punctuation memberships. Combining marks
and controls present in those sources are preserved as component evidence.
Index exemplars retain evidence without automatically declaring alphabet
membership or order. Locale-aware lower/upper/title variants of scalar forms
supply additional candidate memberships (e.g. English `a` -> `A`, Turkish
`i` -> `İ`). Multi-scalar case expansions remain review evidence only and do
not create additional literal constructions or memberships. No three-period
ellipsis is fabricated; `…` remains the single U+2026 endpoint.

`coverage.json` keeps source memberships, distinct source sequences, shared
values including applied casing, actual membership edges and case candidates
as separate counts. Missing inventories, empty main inventories and source
resolution errors remain visible. Preview exits **2** for coverage gaps;
`--allow-gaps` permits a successful preview exit while retaining those gaps.
Malformed UnicodeSets, invalid scalars and conflicting constructions fail.
Aliases support root/section/field redirects, cross-locale sources and relative
or absolute LDML paths with attribute-equality predicates. Unsupported path
syntax and inheritance/alias cycles are reported rather than guessed.

## Read-only core validation

Obtain a plain COPY-format export from the target core, or use a retained
plain/gzip export. The following read-only command exports the three required
tables using PostgreSQL's consistent dump snapshot (use your existing local
connection configuration):

```bash
pg_dump --data-only --no-owner --no-privileges \
  --table=public.token --table=public.token_parent --table=public.token_child \
  --dbname=hcp_core --file=/tmp/hcp-core-inventory.sql
python tools/linguistics/import_inventory.py validate-core \
  --manifest sources/data/linguistics/english-pilot/manifest.json \
  --export /tmp/hcp-core-inventory.sql \
  --out sources/data/linguistics/core-validation.json
```

The validator streams COPY data without executing SQL, restoring a database
or retaining unrelated rows. It checks all 256 floor byte definitions and
the manifest's referenced endpoints/dependencies: addresses, byte notation,
whole structural mass, contiguous ordered parents (including repetitions)
and materialized child reciprocals. ASCII endpoints have nibble parents;
multibyte endpoints have byte parents. Deferred NULL parent-row mass cells
stay NULL; verified constituent token masses supply their expected values.
NULL/incorrect whole mass and incorrect non-NULL parent mass fail validation.
Generated memberships are checked for reciprocal agreement and group cycles;
persisted Linguistics memberships are outside this slice.

Exit **0** means valid within the supplied export, **2** means partial
(missing values) or invalid (conflicting definitions), and **1** means a
source/export error. A missing COPY section or Git LFS pointer is an error.
No current live-store integrity is claimed from an old export. Final writes
remain subject to the owning-store/reference/reciprocal-write/retry contract.

## Reproducible evidence and checks

[Checked-in previews](../../data/linguistics/previews) contain the full English
pilot, six-locale coverage and selected cross-language values, validation
against the current full UTF-8 snapshot, the historical floor report, and the
native address audit. The English pilot references
**84 values**: 52 ASCII letters and 32 punctuation forms, giving 175 deduplicated
membership edges including group/hierarchy edges. The full six-locale preview
preserves **771 source memberships**, 249 distinct source sequences (247 scalar
endpoints plus `ch`/`CH`), 294 shared values including applied case variants,
and 1,579 membership edges. Case candidate counts include unapplied multi-scalar
mappings and are not new-object counts.

The current retained export is the **2026-10-03 full UTF-8 snapshot**, extracted
from PR #111 into [snapshot PR #130](https://github.com/Human-Cognome-Project/human-cognome-project/pull/130).
Patrick confirmed it is current on 2026-10-12. All **286** English pilot
endpoints/dependencies and all **492** six-locale endpoints/dependencies pass
construction, whole-mass and child-reciprocal validation against its exact
payload. Both reports are **valid**, with zero missing values or observed
conflicts. All 14 English punctuation endpoints are present.

The historical 2026-09-28 floor report remains **partial**, because that export
contains only nibbles/bytes and predates the Unicode endpoint population.
It supplies a floor baseline; use the October 3 dump for current inventory
validation. Neither operation connects to a live database.

The **20-language bootstrap** has no inventory-source gaps and preserves
2,387 source memberships. It describes 914 reusable value candidates (27
multi-scalar forms), 65 groups and 5,460 membership edges; 913 values receive
alphabet/punctuation memberships. All 1,082 referenced core endpoints and
dependencies pass against the full UTF-8 snapshot. The checked-in compact
inventory catalog gives each language's alphabet/punctuation member keys and
exact forms. A selection summary includes representative value trees and a
digest covering the complete English/Greek/Hebrew bundle, keeping the larger
generated manifests in the ignored working cache.

```bash
python -m unittest discover -s tools/linguistics/tests -v
python tools/linguistics/evidence.py
python tools/linguistics/evidence.py --bootstrap-only
python tools/linguistics/audit_addresses.py \
  --out sources/data/linguistics/native-audit.json
cmp sources/data/linguistics/native-audit.json \
  data/linguistics/previews/native-address-audit.json
```

`evidence.py` compares newly generated output to repository evidence by
default. `--write` regenerates those files for a reviewed source/behavior
change. **`--utf8-export PATH`** also compares the two current validation
reports; supply the full snapshot gzip blob identified by SHA256
`4408fd55e8884efa6a1b575b7e317aea17bdea922b09fab193cf9f7f65456dc8`.
Use `--bootstrap-only --utf8-export PATH` to compare all four twenty-language
evidence files, including the current-core validation report.
CI fetches the snapshot alone from its pinned extraction commit and runs
this comparison; it does not check out or execute the runtime work in #111.
`--core-export PATH` additionally compares the historical floor report, using
the September 28 gzip blob identified by SHA256
`1bd7a8b073c4b1fe4acdf248c4811275d5da6b9b95817003b92568a05449bc6d`.
Reruns are deterministic; no timestamps or absolute source paths are emitted.

The native audit fetches five hash-pinned files from PR #111 at
`5120ee6e6316bf76b6cd7a91ec51913054464877` and compares address/UTF-8 derivation
for all **1,112,064 valid scalars**. It links only the pure functions: no
controller implementation or libpq. Downloaded files and its binary remain
in the ignored cache. This preserves the original independent preview check
without integrating that unmerged PR's population path or clearing its
record-integrity findings. CI runs the same offline checks and exact evidence
comparison; tests use synthetic exports and never reset a database.
