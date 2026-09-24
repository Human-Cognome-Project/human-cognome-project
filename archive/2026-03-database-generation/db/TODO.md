# Database — TODO

Last updated: 2026-03-06

---

## High Priority

- [ ] **Fix marker table PK collision** — [GitHub #8](https://github.com/Human-Cognome-Project/human-cognome-project/issues/8). Control tokens share (t_p3, t_p4), needs t_p5 column added to `doc_marker_positions`.
- [ ] **Activity envelope implementation** — [GitHub #26](https://github.com/Human-Cognome-Project/human-cognome-project/issues/26). Schema exists (migration 021). Engine-side activation/eviction cycle in HCPEnvelopeManager still needed. Bootstrap envelope policies TBD.
- [ ] **LMDB entity recompile** — compile_entity_lmdb.py needs review before next compile (may not be current with variant/morph system changes).

## Medium Priority

- [ ] **Update boilerplate loading** — [GitHub #9](https://github.com/Human-Cognome-Project/human-cognome-project/issues/9). Remove forward walk, boilerplate pushed to LMDB on cache miss.
- [ ] **Punctuation tokens in hcp_english** — AB.AA.AA.AF.* tokens created in error, needs cleanup to hcp_core.

## Contributor / As Available

- [ ] **Secondary character registration** — supporting/minor characters need entity entries
- [ ] **Dramatis personae** — character lists per work from entity data
- [ ] **Title cleanup** — standardize work titles across entity DBs
- [ ] **Multiple edition deltas** — detect differences between editions
- [ ] **Same text detection** — deduplication across sources/editions

## Backlog

- [ ] Force profile junction table (deferred until PBM experimentation)
- [ ] Fresh DB dumps after any further schema/data changes

---

## Completed (recent)

- [x] Entity normalization — name cleanup (363 fic + 43 nf), position reindexing, 27 place inserts, 6 phantom refs fixed, 34 Labels lowercased (2026-03-05)
- [x] Migration 023 — variant form support: category + canonical_id on tokens (a897569)
- [x] Migration 024 — variant data cleanup: 805 bad auto-populations fixed, 4,815 clean variants (202a184)
- [x] Migrations 017-022: pronoun I, particle_key, freq_rank, position modifiers, activity envelopes, var_sources rename (237eec8)
- [x] DB dump refresh with migrations 016-024 applied (aedc987, ea42aae)
- [x] Frequency rank merge: Wikipedia + OpenSubtitles → Postgres (7f33603)
- [x] LMDB compilation: vocab beds, entity sub-dbs (7f33603)
