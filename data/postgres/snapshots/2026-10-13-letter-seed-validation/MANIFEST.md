# Isolated letter seed population, 2026-10-13

These paired PostgreSQL 16.15 exports preserve the **owned isolated validation
stores**, not an existing user installation. The complete retained October 3
UTF-8 core was restored into a newly created empty fixture; a separate new
Linguistics database used the reviewed passive schema. The bounded writer then
populated the accepted 20-language initial batch. Existing user stores and the
October 3 source export remain unchanged.

| Export | SHA256 |
|---|---|
| `hcp_core.sql.gz` | `f8019624a617a84410c0fd927b4947ed87955b48280016af8fb40b35eb8ac121` |
| `hcp_linguistics.sql.gz` | `9784ed93d361de61bfb169d13a6e2fd9d2a58eb4dd1ac889b9dc6e0d09e32296` |

Both are gzip-compressed plain SQL from `pg_dump --no-owner --no-privileges`;
Git LFS stores the compressed payloads. The source core export SHA256 is
`4408fd55e8884efa6a1b575b7e317aea17bdea922b09fab193cf9f7f65456dc8`.
[COUNTS.json](COUNTS.json) records seed/map/ledger/payload hashes, counts and
order-independent row digest sums. [The retained ledger](../../../linguistics/letter-seed-ledger.json)
maps every symbolic seed identity to its accepted address; the
[acceptance report](../../../linguistics/population-validation.json) covers all checks.

| Table | Core before | Core after | Linguistics |
|---|---:|---:|---:|
| token | 1,112,215 | 1,112,215 | 2,772 |
| token_parent | 4,382,976 | 4,382,976 | 2,741 |
| token_child | 4,333,104 | 4,335,845 | 0 |
| members | 1,112,213 | 1,112,213 | 5,460 |
| member_of | 1,112,213 | 1,112,213 | 5,460 |

Linguistics contains 2,707 language-specific component endpoints and 65 group
anchors. Components have exact source notation, verified whole mass and ordered
core character parents; their parent-row mass cells remain NULL. Group whole
masses remain NULL, with no naming constructions or centroid assembly. Single
Unicode ellipses retain one character parent. Repeated parent occurrences are
covered by a separate test-only sequence, without altering the retained inventory.

Exactly two destination foreign keys differ from the original schema:
`hcp_core.token_child.child_token_id` and
`hcp_linguistics.token_parent.parent_token_id` may now point to the other store.
Owner-token and all membership keys remain. The core's exact original mass
trigger is retained; Linguistics has none. Core tokens, parents, memberships and
every pre-existing child row were preserved. The identical rerun added zero rows.

Read-only reproduction can restore the pair into **new empty databases**. Do not
restore over existing data or use the unchanged native mutation paths on these
cross-linked fixtures. These dumps are not an instruction to modify a user store.
New target population still awaits concrete implementation/schema review and
explicit target selection when using the writer. Patrick will load these paired
exports locally after review; no external cloud connection is needed for that
handoff. The private execution journal is intentionally not
published: it belongs to its original database identities. Preserve the original
journal for recovery there; a restored copy is evidence, not the same transaction.

## Verified local restore

[RESTORE-VALIDATION.json](RESTORE-VALIDATION.json) records a successful restore
of both exact compressed payloads into new empty PostgreSQL 16.15 databases.
Counts, whole masses, NULL cells, ordered parents, membership reciprocals,
core child links, schema/trigger definitions and core row digests pass read-back.
The restore was also read back onto an existing PostgreSQL 16.13 server on
2026-10-20; see [Restoring onto an existing server](#restoring-onto-an-existing-server).

Use a `psql` client of 16.15 or newer (the exports were written by `pg_dump`
16.15, whose `\restrict` / `\unrestrict` lines are `psql` meta-commands). The
server version is a separate matter: the pair was also restored and read back
exactly on an existing PostgreSQL **16.13** server using 16.15 client tools
(2026-10-20). That is an observation, not a guarantee for other versions. From
the repository root, this example creates new databases without replacing
an existing installation. The database names are deployment configuration:
choose them yourself and set `CORE_DB` and `LINGUISTICS_DB` (the placeholders
below) to names that do not yet exist on the server. Ordinary engine use
connects to the configured retained stores; this restore is for recovery or
isolated validation, and creates a separate database only for that explicit
purpose.

```bash
set -euo pipefail
: "${CORE_DB:?set to the new, empty target database name}"
: "${LINGUISTICS_DB:?set to the new, empty target database name}"
git lfs pull --include='data/postgres/snapshots/2026-10-13-letter-seed-validation/*.sql.gz' --exclude=''
createdb --template=template0 --encoding=UTF8 "$CORE_DB"
createdb --template=template0 --encoding=UTF8 "$LINGUISTICS_DB"
gzip -dc data/postgres/snapshots/2026-10-13-letter-seed-validation/hcp_core.sql.gz \
  | psql -X --set=ON_ERROR_STOP=1 --dbname="$CORE_DB"
gzip -dc data/postgres/snapshots/2026-10-13-letter-seed-validation/hcp_linguistics.sql.gz \
  | psql -X --set=ON_ERROR_STOP=1 --dbname="$LINGUISTICS_DB"
```

A database name is deployment configuration; the snapshot filename, date and
label are provenance of the artifact. Neither determines the other, and token
addresses are unchanged by whatever names are chosen. Fetch the Git LFS
payloads before restoring. These exports already contain the seed data and the two reviewed foreign-key changes; do not rerun the
population writer just to load the pair. The bootstrap-only mutation restriction
described above continues to apply to the restored cross-linked stores.

### Restoring onto an existing server

The commands above assume a local default connection. These adjustments cover
mounting the pair on an existing PostgreSQL server, which may be remote or
host-installed. Nothing here relaxes the new-empty-database requirement.

- **Connection settings.** Select the server through libpq configuration rather
  than command-line arguments: a `~/.pg_service.conf` service (`[NAME]` with
  `host`, `port`, `user`), or `PGHOST` / `PGPORT` / `PGUSER`, with the password
  in `~/.pgpass` (mode 600). Never put credentials or private host details in
  Git. Then, for example,
  `createdb --maintenance-db="service=NAME" --template=template0 --encoding=UTF8 "$CORE_DB"`
  and `psql --dbname="service=NAME dbname=$CORE_DB"` (likewise for
  `$LINGUISTICS_DB`).
- **Atomic restore.** Add `--single-transaction` to the `psql` restore commands
  (alongside `ON_ERROR_STOP`). A failed restore then leaves an empty database
  rather than a partial one.
- **Names are local configuration.** Database names are deployment
  configuration; the snapshot filename, date or label is artifact provenance.
  Neither determines the other, and the former `*_seed` suffix was an example
  conflation, not a required namespace or an instruction to create another
  persistent database per snapshot or run. Supply the chosen names at invocation
  (`dbname=` in a conninfo string, the `HCP_*_TEST_CONNINFO` variables, or a
  service entry), never hard-coded in repository code. The protections against
  restoring over populated stores stand: restore only into a new, empty
  database, and if a database with the intended name already exists, choose a
  different name for the new one. This guidance authorizes no rename,
  overwrite, drop or migration of any existing database; any such change is the
  owner's separate decision and is outside this document.

#### Read-back verification

After restoring, compare per-table counts with
[RESTORE-VALIDATION.json](RESTORE-VALIDATION.json) (all ten tables) and, for the
core, the order-independent row digest recorded under `core_after` in
[COUNTS.json](COUNTS.json). This is the method used by `snapshot()` in
[`tools/linguistics/populate.py`](../../../../tools/linguistics/populate.py). For
each of `token`, `token_parent`, `token_child`, `members` and `member_of` on the
restored core:

```sql
SELECT count(*),
  COALESCE(sum(('x'||substr(md5(r::text),1,16))::bit(64)::bigint),0)::text AS h1,
  COALESCE(sum(('x'||substr(md5(r::text),17,16))::bit(64)::bigint),0)::text AS h2
FROM public.token r;  -- repeat for each table name
```

`count`, `h1` and `h2` must equal that table's `core_after` entry exactly. On
the 2026-10-20 mount, every count and all five core digests matched.

See [writer and validation instructions](../../../../docs/letter-population-writer.md).
