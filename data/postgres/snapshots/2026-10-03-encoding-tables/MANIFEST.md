# Snapshot: hcp_core encoding tables — 2026-10-03

Reproducibility snapshot of the `hcp_core` development store after the UTF-8
encoding-tables build-out: the structural-mass trigger, the relabel, the three
category labels, and the full multi-byte character populate. It supersedes the
2026-09-28 encoding-floor snapshot (which held only the floor).

## Extraction and read-only verification

The dump and checksum are copied byte-for-byte from
[PR #111](https://github.com/Human-Cognome-Project/human-cognome-project/pull/111)
at `5120ee6e6316bf76b6cd7a91ec51913054464877`. Patrick confirmed on
2026-10-12 that this populated export is the current core snapshot. The native
controller, trigger implementation and populate-driver changes remain in
that separate PR. This snapshot includes their exported schema definitions
for reproducibility; read-only inspection executes no SQL.

[COVERAGE.json](COVERAGE.json) records an independent scan of the actual dump:
all **1,112,064 valid scalar endpoints** are present (128 ASCII values reuse
byte endpoints; 1,111,936 have multibyte definitions), with exact UTF-8 notation
and whole structural mass, all 256 bytes and 16 nibbles, and the row totals
below. Missing valid scalars and observed value errors are zero.
The audit counts relationship rows; ordered parent/child verification for
the linguistic inventories is recorded separately in PR #129.

## Database
- **Name:** `hcp_core`
- **Export date:** 2026-10-03
- **Engine:** PostgreSQL 16
- **Tool:** `pg_dump 16`, plain format, `--no-owner --no-privileges`, `gzip -9`
- **Schema context:** `kernels/database/schema/schema.sql` — five tables
  (`token`, `token_parent`, `token_child`, `members`, `member_of`) **plus the one
  authorized trigger** `token_parent_structural_mass` (`AFTER INSERT ON
  token_parent`): rejects a NULL-mass parent, else sets the child's `token.mass`
  to the sum of its direct parents' mass. Address is a `text[]` of byte-ordered
  Base62 couplets, `COLLATE "C"`.

## Contents
- **Floor — 273 tokens at `00.00.00.00.*`** (unchanged from the 2026-09-28
  snapshot): 16 nibble atoms (notation `0`–`F`, mass 1), `0x` (mass 0), 256 byte
  codes `00`–`FF` (mass 2, two nibble parents).
- **Labels — 6 at `00.00.01.00.0*`:** `Nibbles`, `Byte Codes`, `Byte Code Groups`
  (the three originals, relabelled from Single Hex Code / Hex Couplets / Hex Code
  Patterns) + `Two-Byte Codes`, `Three-Byte Codes`, `Four-Byte Codes`. The three
  new category labels carry a temporary placeholder `mass` of 10 and are
  `member_of` `Byte Code Groups`; proper masses await their naming literals.
- **Characters — 1,111,936:** every valid UTF-8 codepoint U+0080–U+10FFFF,
  surrogates U+D800–U+DFFF skipped. Each is minted over its UTF-8 bytes as ordered
  byte-code parents; structural `mass` is set by the trigger (2-byte 4, 3-byte 6,
  4-byte 8); each is `member_of` its byte-width category label; temporary
  `notation` is the UTF-8 bytes as uppercase hex (e.g. `C3A9`), not the character.
  Addresses: 2-byte `00.00.00.01.*`–`02.*`, 3-byte `…03.*`–`0z.*`, 4-byte
  `…10.*`–`zz.*`.

## Row totals
| table | rows |
|---|---|
| token | 1,112,215 |
| token_parent | 4,382,976 |
| token_child | 4,333,104 |
| members | 1,112,213 |
| member_of | 1,112,213 |

(`token_child` is below `token_parent` by design: it is one reverse edge per
distinct parent→child pair, so a byte repeated at several ordinals within one
character contributes a single child edge.)

## Read-only reproduction

Retrieve the LFS payload and audit it directly, without restoring a database:

```bash
git lfs pull --include='data/postgres/snapshots/2026-10-03-encoding-tables/hcp_core.sql.gz' --exclude=''
```

From this snapshot directory:

```bash
sha256sum -c hcp_core.sql.gz.sha256
python verify_utf8_snapshot.py hcp_core.sql.gz --out /tmp/hcp-core-utf8-coverage.json
cmp /tmp/hcp-core-utf8-coverage.json COVERAGE.json
```

The verifier uses the Python standard library and streams COPY data. It
checks the address allocation, every scalar's presence, UTF-8 notation and
whole mass, plus table totals. Placeholder label construction and global
relationship integrity retain their separate scope. The source PR reports
a restore check on 2026-10-03; this extraction performs no restore or live
database operation. Any future restore needs its own explicitly selected
empty destination and reviewed schema/trigger procedure.

## Integrity
`hcp_core.sql.gz.sha256` holds the SHA-256 of the dump.

## Dependencies / notes
- PostgreSQL 16; the `COLLATE "C"` pin on address columns is required for address
  order to equal PK order.
- The compressed dump is stored via Git LFS.
- No credentials or private data; development store only.
