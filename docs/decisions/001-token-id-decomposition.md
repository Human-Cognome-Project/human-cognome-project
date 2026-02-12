# Decision 001: Token ID Decomposition

**Date:** 2026-02-12
**Status:** Implemented and migrated
**Shards affected:** hcp_core, hcp_english, hcp_names

## Context

Token IDs (base-50 dotted notation like `AB.AB.CA.Ec.xn`) were stored as monolithic TEXT strings. Each ID is 1-5 dot-separated pairs where each pair is a 2-character base-50 value. The pairs form a hierarchical address: namespace, category, sub-category, group, element.

Monolithic storage meant:
- No B-tree prefix compression (millions of tokens sharing `AB.AB` stored the full prefix per row)
- Cross-analysis required string parsing (`split_part`, regex) instead of indexed column queries
- No clean decomposition boundary for LMDB export
- Tree assembly couldn't leverage the schema's natural hierarchy

## Decision

Decompose into 5 TEXT columns + a generated TEXT PK:

```sql
ns   TEXT NOT NULL,       -- pair 1: mode/namespace
p2   TEXT,                -- pair 2: category (NULL for short tokens)
p3   TEXT,                -- pair 3: sub-category
p4   TEXT,                -- pair 4: token group
p5   TEXT,                -- pair 5: atomic element

token_id TEXT GENERATED ALWAYS AS (
    ns || COALESCE('.' || p2, '') || COALESCE('.' || p3, '') ||
    COALESCE('.' || p4, '') || COALESCE('.' || p5, '')
) STORED NOT NULL,

PRIMARY KEY (token_id)
```

With a compound B-tree index: `CREATE INDEX idx_tokens_prefix ON tokens(ns, p2, p3, p4, p5);`

This pattern is **identical across all shards**. Only the shard-specific data columns differ.

## Alternatives Considered

### A. Five columns with composite PK `(ns, p2, p3, p4, p5)`
Rejected. Variable-depth tokens (e.g. `yA.Ap.Jj` = 3 pairs) leave p4/p5 NULL. NULLs in composite PKs cause PostgreSQL to treat each NULL as distinct, silently breaking uniqueness. Also creates 5-column FKs everywhere.

### B. Five columns with composite PK (some shards) + generated PK (other shards)
This is what the old migration files actually did — hcp_core used generated PK, hcp_english used composite PK. Rejected for inconsistency. Cross-shard tooling and application code can't rely on a single pattern.

### C. TEXT[] array PK
Rejected. PostgreSQL GIN indexes on arrays support containment queries, not ordered prefix scans. The claimed B-tree compression benefit doesn't materialize with array storage.

### D. Prefix elimination (drop common prefix within shard)
Considered: since hcp_english tokens are all `AB.AB.*`, store only `p3.p4.p5` within the shard. Rejected for now — adds complexity for cross-shard token references (PBMs reference tokens from multiple namespaces). The B-tree compound index achieves similar compression without schema complexity.

### E. CHAR(2) columns
Initially approved, but caused PostgreSQL `GENERATED ALWAYS AS` immutability errors. The implicit `bpchar→text` cast in the generated expression is not guaranteed IMMUTABLE. Switched to TEXT with documentation that pairs are always exactly 2 characters.

## PostgreSQL Gotchas Encountered

1. **`concat_ws()` is STABLE, not IMMUTABLE.** Cannot use in `GENERATED ALWAYS AS` columns. Solution: `ns || COALESCE('.' || p2, '') || ...` which uses only IMMUTABLE operators.

2. **`CHAR(n)` implicit cast issues.** The `bpchar→text` cast in generated column expressions can trigger "generation expression is not immutable." Solution: use TEXT columns.

3. **CASCADE on DROP TABLE.** Dropping the tokens table cascades to FK constraints on other tables (e.g. `entries.word_token → tokens.token_id` in hcp_names). Migration must restore these FKs after recreating the table.

## Migration Details

Files: `db/migrations/000_helpers.sql` through `003_names.sql` + `run.sh`

Each migration:
1. Backs up to temp table
2. Drops and recreates with new schema
3. Migrates data via `CROSS JOIN LATERAL split_token_id()`
4. Verifies row count AND token_id reconstruction (JOIN backup on token_id)
5. Restores cascaded FKs where needed
6. Runs ANALYZE

### Results (2026-02-12)

| Shard | Tokens | Namespace | Notes |
|-------|--------|-----------|-------|
| hcp_core | 5,234 | AA | pbm_entries/scopes removed, shard_registry added |
| hcp_english | 1,252,854 | AB (1,252,814 in AB.AB) | 27 distinct p3 values |
| hcp_names | 150,528 | yA | 3-pair depth (p4/p5 NULL) |

### Shard Registry

Created in hcp_core as canonical namespace-to-database routing:

| ns_prefix | shard_db | description |
|-----------|----------|-------------|
| AA | hcp_core | Universal/computational |
| AB | hcp_english | English language family |
| yA | hcp_names | Name components |
| zA | hcp_en_pbm | Source PBMs (experimental) |

## Files Removed

Old superseded migration files deleted from `db/`:
- `migrate_to_five_column_tokens.sql`, `migrate_core_to_five_column.sql`, `migrate_english_to_five_column.sql`, `migrate_names_to_five_column.sql`, `migrate_pbm_to_five_column.sql` (inconsistent 5-column approaches)
- `migrate_to_atomic_tokens.sql` (TEXT[] approach)
- `run_all_migrations.sh` (old runner)

Retained in `db/`:
- `core.sql`, `english.sql`, `names.sql` — pre-migration dumps (still useful for fresh loads before migration)
- `hcp_en_pbm_schema.sql` — PBM schema reference (experimental, in-progress)
- `pbm_store.sql` — experimental SQLite PBM schema (different paradigm, not superseded)
- `spacing_rules.sql` — standalone spacing rules feature
- `load.sh` — dump loader (updated to include names)
- `tools/` — token generation utilities
