# Database TODO

## High Priority
- [ ] **Fix marker table PK collision** — control tokens share (t_p3, t_p4), needs t_p5 column added
- [ ] **Update boilerplate loading** — remove forward walk, boilerplate pushed to LMDB on word cache miss
- [ ] **Space-to-space concatenation in boilerplate** — Postgres must reconstruct chunks from decomposed tokens when matching boilerplate

## Medium Priority
- [ ] **Document Thing entities** — wA.DA.* entities for Gutenberg works (librarian task, schema ready)
- [ ] **LMDB purge strategy** — background Python process monitors environments, evicts stale entries
- [ ] **Confirm forward table populated** — verify data before removing forward sub-db
- [ ] **Review position map product format** — blocked on engine Phase 1

## Backlog
- [ ] B's team fork review when they pull upstream
- [ ] Force profile junction table (deferred until PBM experimentation)
- [ ] Python code audit in src/hcp/ for decomposed queries
- [ ] Punctuation tokens in hcp_english (AB.AA.AA.AF.*) — created in error, needs cleanup to hcp_core
- [ ] Fresh English dump after any further token additions

## Completed
- [x] Migration 006: force patterns (9ed3e53, afb555e)
- [x] Migration 007: 3 new databases, 152 tokens (0c5f26b)
- [x] Migration 008: "man"/"sun" in hcp_english (4db2b02)
- [x] Migration 009: positional storage (ebcb534) — superseded by 011
- [x] Migration 010: hcp_var database (5d96e4d)
- [x] Migration 011: PBM prefix tree (63a2fe1)
- [x] Cache miss resolver Python reference (5d96e4d)
- [x] C++ resolver spec with handler registry (6a88a74)
- [x] LMDB contract: 7 sub-dbs, plain UTF-8 (dbbac1e)
- [x] var_request token AA.AE.AF.AA.AC (8a23ca2)
- [x] Fresh dumps all 6 DBs (718eba7)
