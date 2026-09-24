# Database — Agent Guide

## Ownership
The DB specialist owns all PostgreSQL schema, migrations, LMDB export format, and vocabulary management.

## Do NOT Touch (without DB specialist coordination)
- Migration files in `db/migrations/` — append-only, never modify existing
- Schema files (`*.sql` at `db/` root) — these are reference schemas
- LMDB sub-database format or key conventions
- Token ID assignment or namespace allocation

## Schema Pattern
Decomposed TEXT columns (ns, p2, p3, p4, p5) + generated TEXT PK `token_id`. Compound B-tree index for prefix compression. Single-column TEXT PK for FKs/JOINs/LMDB export. ALL references decomposed — single refs as column sets, arrays as junction tables with position column.

## Key Conventions
- All DB connectors use **psycopg v3** (`import psycopg`) — NOT psycopg2
- `concat_ws()` is STABLE, not IMMUTABLE — use `||` with `COALESCE` for generated columns
- `CHAR(n)` implicit casts cause immutability issues — use TEXT with comments
- Log big results to files, never dump to stdout
- DB access: user `hcp`, password `hcp_dev` (dev environment)

## How to Add a Migration
1. Create `db/migrations/NNN_descriptive_name.sql`
2. Follow existing patterns (see 011 for current style)
3. Test against dev databases
4. Update the corresponding schema file at `db/` root
5. Export fresh dumps after migration

## Contributor Tasks
See [TODO.md](TODO.md) for current task list. Good first issues:
- Fix marker table PK collision (add t_p5 column)
- Write a migration for a new language shard (use hcp_english as template)
- Performance profiling with EXPLAIN ANALYZE on larger datasets
