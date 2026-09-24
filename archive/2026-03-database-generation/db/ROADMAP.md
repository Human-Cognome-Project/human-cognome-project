# Database Roadmap

PostgreSQL is the source of truth. LMDB is the self-filling runtime cache. Schema design prioritizes portability — dumps must work with lightweight SQL engines, not just PostgreSQL.

## Current State

### Databases (6 active)
- **hcp_core** (AA) — 5,411 tokens. Encoding, structural markers, force infrastructure, namespace allocations.
- **hcp_english** (AB) — 1,396,117 tokens. Words, affixes, derivatives, multi-word expressions.
- **hcp_fic_pbm** (zA equiv) — PBM prefix tree storage (migration 011). Per-document bond maps.
- **hcp_fic_entities** — Fiction entity catalog (People, Places, Things).
- **hcp_nf_entities** — Non-fiction entity catalog.
- **hcp_var** — Var token scratch pad (migration 010). Short-term memory for unresolved sequences.

### LMDB Cache
- Self-filling: starts empty, Postgres resolves cache misses
- 7 sub-databases: w2t, c2t, l2t, t2w, t2c, forward (being removed), meta
- All values: plain UTF-8 (zero-copy C++ mmap reads)
- Contract documented in `docs/questions-for-db-specialist.md`

### Migrations
24 migration files (000-011). Key recent ones:
- 009: Position storage (scaffolding, superseded by 011)
- 010: Var database
- 011: PBM prefix tree (replaces positional storage)

## Phases

### Phase 1 (current): Support Source Workstation
- Confirm Postgres read access patterns for engine
- Fix marker table PK collision (t_p5 column)
- Finalize boilerplate loading (push on cache miss, not forward walk)

### Phase 2: Storage Optimization
- PBM prefix tree tuning based on real workload data
- LMDB purge strategy (background process for cache management)
- Bond table persistence (compile once, load from file)

### Phase 3: Multi-Document Operations
- Cross-document PBM aggregation
- Sub-PBM detection and storage
- Boilerplate library management

### Future: Deployment Models
- Full DB download (PostgreSQL dumps)
- Token ID only (custom assembly via SQLite)
- No DB (preload from remote endpoint on demand)
- Delta sync via created_at/updated_at timestamps

## Contributor Expansion Points

| Task | Difficulty | Prerequisites |
|------|-----------|---------------|
| Fix marker table PK collision | Easy | SQL, understand token addressing |
| Write migration for new language shard | Medium | Schema pattern knowledge |
| SQLite backend implementation | Medium | SQLite + existing schema pattern |
| LMDB purge strategy design | Medium | Understand activity envelopes |
| Performance profiling on larger datasets | Medium | PostgreSQL + EXPLAIN ANALYZE |
