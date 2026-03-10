# Envelope Vocab Spec — DB Specialist Brief

**Date:** 2026-03-10
**Context:** Engine refactored to load PBD vocab from LMDB `w2t` populated by EnvelopeManager. Python LMDB compilers deleted. This spec describes what needs to be done in Postgres to make the vocab pipeline work.

---

## What the engine does

1. Client calls `source_activate_envelope <name>` on the socket
2. `HCPEnvelopeManager::ActivateEnvelope` reads the envelope definition and its queries from `hcp_core`
3. For each query: connects to the target shard DB (`shard_db` field), runs the SQL, writes `col[0] → col[1]` to the target LMDB sub-db
4. `BedManager::RebuildVocab()` cursor-scans LMDB `w2t`, builds in-memory vocab index per word length
5. `BedManager::Resolve` runs CharRuns through PBD against that vocab

The engine reads exactly one LMDB sub-db for vocab: **`w2t`** (key = word form, value = token_id).

---

## Issues with existing envelope_queries (must fix)

The two envelope definitions (`english_common_10k`, `fiction_victorian`) and their 16 queries exist in `hcp_core` but have the following bugs:

### 1. Wrong column names

The queries reference columns that don't exist in `hcp_english.tokens`:

| Query uses | Actual column | Fix |
|---|---|---|
| `word` | `name` | `SELECT lower(name) AS word, token_id FROM tokens ...` |
| `frequency_rank` | `freq_rank` | `ORDER BY freq_rank ASC` |

### 2. Wrong LMDB target sub-db

All 15 existing queries write to `env_w_02`, `env_w_03`, ..., `env_w_16` (one per word length).
The engine reads from **`w2t`**.

All vocab queries need `lmdb_subdb = 'w2t'`. The per-length sub-dbs are unused and can be dropped from the queries.

### 3. Category filter wrong in fiction_victorian

Query #2 uses `category IN ('archaic', 'literary', 'formal')`.
`formal` does not exist in the data. Valid categories are: `archaic` (4579), `dialect` (143), `casual` (74), `literary` (19), NULL (1,390,687 — the main vocabulary).

---

## What the queries should look like

### Envelope: `english_common_10k`

Replace the 15 per-length queries with a single query (or keep per-priority if ordering matters):

```sql
SELECT lower(name) AS word, token_id
FROM tokens
WHERE freq_rank IS NOT NULL
  AND length(name) BETWEEN 2 AND 16
ORDER BY freq_rank ASC
```

Target: `lmdb_subdb = 'w2t'`

**Or** keep per-length for priority ordering (load short common words before long ones):

```sql
-- Priority 2: 2-letter words
SELECT lower(name), token_id FROM tokens
WHERE length(name) = 2 AND freq_rank IS NOT NULL
ORDER BY freq_rank ASC

-- Priority 3: 3-letter words
SELECT lower(name), token_id FROM tokens
WHERE length(name) = 3 AND freq_rank IS NOT NULL
ORDER BY freq_rank ASC LIMIT 5000

-- ... etc through length 16, all targeting w2t
```

### Envelope: `fiction_victorian`

Fix the extra vocab query (same column renames, same target):

```sql
SELECT lower(name), token_id
FROM tokens
WHERE category IN ('archaic', 'literary', 'formal', 'dialect')
ORDER BY freq_rank ASC NULLS LAST
```

Target: `lmdb_subdb = 'w2t'`

Note: `formal` category may not be populated yet — leave it in the filter for forward compatibility.

---

## Questions for the DB specialist

**Q1. Eviction on envelope switch**
When `DeactivateEnvelope` fires, `EvictManifest` drops all sub-dbs listed in the manifest for that envelope. If all queries target `w2t`, the entire `w2t` sub-db gets dropped on eviction — including any entries written by the cache miss resolver during the session. Is this the intended behavior (full turnover on switch), or should the manifest track individual keys rather than whole sub-dbs?

**Q2. `shard_registry` usage**
`GetShardConnection` in the engine currently hardcodes `host=localhost dbname=<shardDb> user=hcp password=hcp_dev`. Should it look up the connection string from `shard_registry` instead? What is the current schema/purpose of `shard_registry`?

**Q3. Labels (proper nouns) — future**
The `proper_common` column is empty. When Labels are populated, how will they be identified — by `proper_common`, by namespace (`ns`), by layer? The engine needs to distinguish Label-tier vocab for the capitalization broadphase. Not a blocker now, but needs a plan.

---

## Summary of required changes (blockers)

| # | What | Where |
|---|------|--------|
| 1 | Fix `word` → `name` in all query_text entries | `hcp_core.envelope_queries` |
| 2 | Fix `frequency_rank` → `freq_rank` in all query_text entries | `hcp_core.envelope_queries` |
| 3 | Fix `lmdb_subdb` from `env_w_XX` → `w2t` for all queries | `hcp_core.envelope_queries` |
