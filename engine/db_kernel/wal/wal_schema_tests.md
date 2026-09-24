# wal_manager schema — coordinator checklist

Concrete checks to run after applying `wal_schema.sql`, using
`wal_verify.sql` (section numbers match `wal_verify.sql`'s `-- N.`
headers).

1. **Tables exist.** Query 1 returns exactly 2 rows, both as `BASE TABLE`:
   `history`, `obligation`. Fail if either is missing, or either shows as
   anything other than `BASE TABLE`.

2. **`obligation` shape.**
   - 5 columns, all NOT NULL: `kind` (text), `addr_a` (array),
     `addr_b` (array), `source` (text), `lsn` (bigint).
   - PRIMARY KEY is exactly `(kind, addr_a, addr_b)`, in that order — this
     is the FIXED two-slot identity (structure `(parent, child)`,
     membership `(member, group)`, mass `(token_id, '{}')`), matching
     `schema/schema.sql`'s address-column pin so PK order equals address
     order.
   - CHECK constraint `obligation_kind_valid` exists, restricting `kind`
     to `structure` / `membership` / `mass`.
   - The follow-up query returns 0 rows: there is **no** `status`, `done`,
     or `seq` column. "Open = membership" — a row's mere existence is the
     open signal; closing is a PK DELETE, never a flag flip
     (WAL-IMPL-PLAN.md Spec 2).

3. **`history` shape.**
   - 7 columns: `source` (text, NOT NULL), `lsn` (bigint, NOT NULL),
     `footprint` (array, NOT NULL, default `'{}'`), `scope` (text, NOT
     NULL), `settled_kind` (text, **nullable**), `settled_addr_a` (array,
     **nullable**), `settled_addr_b` (array, **nullable**).
   - PRIMARY KEY is exactly `(source, lsn)`, in that order — one row per
     report, per WAL-PLAN.md §3's per-write report model.
   - Exactly 3 CHECK constraints (excluding Postgres's own
     auto-generated NOT NULL pseudo-CHECK constraints, a PG 12+ catalog
     artifact of each column's NOT NULL rather than a declared
     constraint): `history_scope_valid` (`scope` in `local`/`global`),
     `history_settled_kind_valid` (`settled_kind` NULL or one of
     `structure`/`membership`/`mass`), and
     `history_settled_identity_shape` (all three `settled_*` columns are
     NULL together, or all three are present together — no half-recorded
     settlement).

4. **Address columns carry `COLLATE "C"`.** Query 4's first check returns
   exactly 4 rows, all `collname` = `C`: `obligation.addr_a`,
   `obligation.addr_b`, `history.settled_addr_a`,
   `history.settled_addr_b`. This is the same fix as
   `schema/schema.sql`'s address-column collation pin — it keeps PK/order
   comparisons on these identity columns aligned with `codec::kAlphabet`'s
   byte order, matching `schema/schema.sql`'s own address columns. The
   second check (`history.footprint`, database default collation) returns
   0 rows — `footprint` is dot-joined debug text (like
   `schema/schema.sql`'s `token.token_text`), not an address column, and
   must stay on the default collation, never pinned to `"C"`.

5. **Mass sentinel is non-null-shaped.** Query 5 returns 0 rows: `addr_b`
   is NOT NULL. This is what makes the mass obligation's sentinel `'{}'`
   (an empty, non-null `text[]`) the only way to represent "no second
   axis" — a NULL is never possible there, so mass stays a normal row of
   the same PK shape as structure/membership, in the SAME `obligation`
   relation (Spec 4 / WAL-PLAN.md §5) — no separate mass table.

6. **No triggers/functions.** Query 6 returns 0 rows across both tables —
   confirms no derivation logic snuck into the schema layer; recognition,
   matching, and monitoring all belong to the C++ layer.

7. **PK indexes only.** Query 7 lists every index across both tables;
   every `indexname` must end in `_pkey`. In particular, confirm there is
   **no** index on `obligation.source` (a source-filtered `list_open`
   would be a forbidden non-PK scan — WAL-IMPL-PLAN.md W-4) and **no**
   secondary index of any kind on `history` beyond its own PK — the WAL
   manager only-follows; it never reverse-searches.

## Build/run

```bash
createdb wal_manager 2>/dev/null || true
psql -d wal_manager -v ON_ERROR_STOP=1 -f wal_schema.sql
psql -d wal_manager -v ON_ERROR_STOP=1 -f wal_verify.sql
```

Re-run against a freshly recreated `wal_manager` (`dropdb wal_manager &&
createdb wal_manager`) to confirm the schema loads clean from scratch each
time (idempotent-from-clean, not idempotent-in-place — `wal_schema.sql`
issues plain `CREATE TABLE`, so a second run against the same live
database is expected to error on already-existing objects, same as
`schema/schema.sql`).

`grep -i 'create index' wal_schema.sql` should show no matches: every
index in this schema is PK-implied, never an explicit `CREATE INDEX`.
