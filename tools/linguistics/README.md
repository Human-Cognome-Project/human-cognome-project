# Bulk linguistic bootstrap

**BUILT:** pinned Unicode/CLDR acquisition, ICU UnicodeSet parsing and locale
casing, source inheritance/aliases, distinct linguistic endpoint candidates,
ordered core parents and reciprocal child additions, symbolic groups,
reciprocal memberships, coverage reports and a read-only COPY-export
validator. The bounded population writer now implements the accepted initial
address map, explicit whole masses, additive core child listings, two-store
prepared transactions and durable same-batch recovery. This Python tooling
only prepares, populates and checks early bulk bootstrap data.

**REVIEW BOUNDARY:** existing user stores await concrete implementation/schema
review and explicit target connections. Native group-centroid assembly remains
planned. Group
centroid descriptions are requirements with pending state; no coordinates or
aggregate masses are calculated here. The manifest is a proposal, not a
write-ready population batch until its retained ledger and selected core are verified.

The purpose is functional bootstrapping: seed a connected representational
framework around which subsequent structure can form. UTF-8 is the encoding
target for this stage. Once sufficient density supports viable interaction,
further material and other encoding-table qualification follow the standard
methods. The current test objects are individual letters in each language;
this work supplies their bulk seed definitions and memberships.

The [accepted plan](../../docs/language-component-unicode-plan.md) defines the
membership/construction contract. Each language's letter/punctuation endpoint
has its own linguistic identity and ordered `hcp_core` character parents,
including single-scalar letters. Each core parent receives the corresponding
language endpoints in its child listing. Repetitions, case and
combining sequences are preserved, without normalization. Each alphabet and
punctuation grouping has its own pending centroid description. Language
hierarchy edges are explicit reviewed bootstrap metadata, separate from CLDR
source inheritance and lexical fallback preference. Concrete linguistic
addresses are retained in [letter-seed-ledger.json](../../data/linguistics/letter-seed-ledger.json).

## Requirements and pinned sources

Use Python 3.10+ and a JDK 21+ with its source-file launcher. The importer uses
the standard library and the pinned ICU4J 78.1 jar; it does not use the host's
Python/system Unicode tables. The native derivation audit additionally needs
`c++` with C++17 and GNU-compatible section garbage collection (Linux).
Source preparation needs no PostgreSQL service or credentials. Population needs
PostgreSQL 16, `psql`, two explicit databases in the same cluster, a durable
private journal directory and `max_prepared_transactions >= 2`. The test helper
uses pinned PostgreSQL 16.15 through Docker, Unix sockets and peer authentication;
it exposes no network listener.

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

## Candidate output

The v2 `manifest.json` separates:

- `source_values`: shared core character references and source sequence
  evidence. Index-only forms stay here without becoming linguistic endpoints.
- `values`: distinct language-scoped letter/punctuation endpoints, with exact
  forms, source references and ordered core character parents. English `A` and
  French `A` have distinct seed keys and the same existing core `A` parent.
- `core_child_updates`: additive child listings for every referenced core
  character. Each listing names its distinct linguistic children and their
  destination store. Repeated parents retain their ordinals in `values`, with
  one reciprocal child edge per core/linguistic endpoint pair. Existing core
  child listings are preserved. Missing, duplicate or unrelated additions,
  replacement updates and omitted letter parents fail seed validation.
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
ellipsis is fabricated; a language's `…` endpoint has the single core U+2026
endpoint as its parent.

`coverage.json` keeps source memberships, distinct source sequences, source
values including applied casing, linguistic endpoints, parent occurrences,
core child edges, actual membership edges and case candidates
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
candidate core child additions are checked against all linguistic parents.
The supplied export validates the existing core construction; it does not
claim that the new linguistic child entries have already been persisted.

Exit **0** means valid within the supplied export, **2** means partial
(missing values) or invalid (conflicting definitions), and **1** means a
source/export error. A missing COPY section or Git LFS pointer is an error.
No current live-store integrity is claimed from an old export. Final writes
remain subject to the owning-store/reference/reciprocal-write/retry contract.

## Reproducible evidence and checks

[Checked-in previews](../../data/linguistics/previews) contain the full English
pilot, six-locale coverage and selected cross-language values, validation
against the current full UTF-8 snapshot, the historical floor report, and the
native address audit. The English pilot has
**84 linguistic endpoints**: 52 ASCII letters and 32 punctuation forms, each
with its existing core character parent and reciprocal child addition, giving
175 deduplicated membership edges including group/hierarchy edges. The full six-locale preview
preserves **771 source memberships**, 249 distinct source sequences (247 scalar
source forms plus `ch`/`CH`), 294 source values including applied case variants,
781 linguistic endpoints, 782 core child additions and 1,579 membership edges.
Case candidate counts include unapplied multi-scalar
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
2,387 source memberships and 914 source values (27 multi-scalar forms).
It has **2,707 language-scoped endpoints**, 65 groups and 5,460 membership
edges. **2,741 ordered core parent occurrences** have **2,741 reciprocal child
additions** across 890 core endpoints. All 1,082 referenced core endpoints and
dependencies pass against the full UTF-8 snapshot. The checked-in compact
inventory catalog gives each language's alphabet/punctuation member keys,
exact forms, parents and additive core child updates. The full generated bulk
candidate manifest stays in the ignored
working cache. This batch is database seed preparation; the generic Python
membership selector and working-area bundle format have been removed.

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
Use `--bootstrap-only --utf8-export PATH` to compare all three twenty-language
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

## Persistent bootstrap validation

The [writer instructions](../../docs/letter-population-writer.md) explain explicit
target selection, the two destination foreign-key changes, mass policy, offline
mutation restriction and durable recovery. The
[isolated acceptance report](../../data/linguistics/population-validation.json)
includes the full retained core load, exact read-back, a zero-row rerun, conflict
rejection and prepare/commit interruption recovery. The
[paired exports](../../data/postgres/snapshots/2026-10-13-letter-seed-validation/MANIFEST.md)
preserve the populated test stores. Existing user stores have not been changed.
Hosted CI also reproduces the isolated PostgreSQL load; no database reset is used.
