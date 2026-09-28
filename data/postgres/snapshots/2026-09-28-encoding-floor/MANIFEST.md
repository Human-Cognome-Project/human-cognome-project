# Snapshot: hcp_core encoding floor — 2026-09-28

Reproducibility snapshot of the `hcp_core` development store after seeding the
hex encoding floor.

## Database
- **Name:** `hcp_core`
- **Export date:** 2026-09-28
- **Engine:** PostgreSQL 16
- **Tool:** `pg_dump 16.15`, plain format, `--no-owner --no-privileges`, `gzip -9`
- **Schema context:** `kernels/database/schema/schema.sql` — five tables
  (`token`, `token_parent`, `token_child`, `members`, `member_of`); address is a
  `text[]` of byte-ordered Base62 couplets, address columns pinned `COLLATE "C"`
  (see PR #101 for the codec).

## Contents — the encoding floor
- **17 base particles** at `00.00.00.00.0*`: 16 hex atoms `…00`–`…0F`
  (notation `0`–`F`, mass 1) + `0x` virtual root at `…0G` (notation `0x`, mass 0).
- **3 provisional label rows** at `00.00.01.00.0*` (assigned mass 10, notation = visible
  value): `Single Hex Code`, `Hex Couplets`, `Hex Code Patterns`. `Single Hex
  Code` and `Hex Couplets` are members of `Hex Code Patterns`. The 16 hex codes
  are members of `Single Hex Code`.
- **256 hex couplets** (bytes `0x00`–`0xFF`) at `00.00.00.00.10`–`…57`; the
  `0H`–`0z` tail of the base ring is left sparse (the group-sparsity boundary).
  Each couplet: ordered parents = high then low nibble (both single hex codes),
  folded-and-wired into `token_child`; mass 2 (sum of parents); a member of
  `Hex Couplets`.

These labels are placeholders, not fully constructed tokens. Their masses
have been assigned, but the data needed to form their proper structures does
not exist yet. They have no parents/children; prose `notation` stands in for
the real naming literal. The `notation` column is temporary/debug and slated
to be dropped.

## Row totals
| table | rows |
|---|---|
| token | 276 |
| token_parent | 512 |
| token_child | 496 |
| members | 274 |
| member_of | 274 |

## Restore
Restore only to an explicitly chosen **empty** database. Do not pipe this dump
into the existing `hcp_core` store: `createdb hcp_core` failing because it
already exists would not stop a following pipeline from writing to it.
Use `psql -X -v ON_ERROR_STOP=1` so a restore fails on its first SQL error.
The exporter verified a separate restore on 2026-09-28 and reproduced the
row totals above.

## Integrity
`hcp_core.sql.gz.sha256` holds the SHA-256 of the dump.

## Dependencies / notes
- PostgreSQL 16; the `COLLATE "C"` pin on address columns is required for address
  order to equal PK order (the gather prerequisite).
- No credentials or private data; development store only.
