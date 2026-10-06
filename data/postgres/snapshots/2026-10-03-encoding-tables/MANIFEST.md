# Snapshot: hcp_core encoding tables — 2026-10-03

Reproducibility snapshot of the `hcp_core` development store after the UTF-8
encoding-tables build-out: the structural-mass trigger, the relabel, the three
category labels, and the full multi-byte character populate. It supersedes the
2026-09-28 encoding-floor snapshot (which held only the floor).

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

## Restore
```
createdb hcp_core
zcat hcp_core.sql.gz | psql -v ON_ERROR_STOP=1 hcp_core
```
Verified 2026-10-03 by restoring into a throwaway database and reproducing the
row totals above. Note: on a plain restore the `token_parent_structural_mass`
trigger re-fires for every `token_parent` row and recomputes the same masses —
correct, but slower; for a faster load, disable triggers for the session
(`SET session_replication_role = replica;`) so the stored masses load as-is.

## Integrity
`hcp_core.sql.gz.sha256` holds the SHA-256 of the dump.

## Dependencies / notes
- PostgreSQL 16; the `COLLATE "C"` pin on address columns is required for address
  order to equal PK order.
- The compressed dump is stored via Git LFS.
- No credentials or private data; development store only.
