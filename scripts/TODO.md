# Scripts — TODO

Last updated: 2026-03-17

---

## Current Scripts

| Script | Purpose | Status |
|--------|---------|--------|
| `merge_frequency_ranks.py` | Merge Wikipedia + OpenSubtitles frequency data into Postgres | Working |
| `run_benchmark.py` | Benchmark runner (Sherlock/Dracula/Scarlet) | Working |
| `ingest_texts.py` | Text ingestion via socket API | Working |
| `hcp_client.py` | Socket API client library | Working |

**Note**: `compile_vocab_lmdb.py` and `compile_entity_lmdb.py` previously referenced here are no longer needed — LMDB is now populated via the envelope system at runtime (`EnvelopeManager::ExecuteQuery`). Kaikki pass scripts (pass1-6) moved to `scripts/deprecated/`.

## Improvements

- [ ] **SQLite export script** — Export vocabulary to SQLite format for standalone workstation distribution. Ties to [GitHub #19](https://github.com/Human-Cognome-Project/human-cognome-project/issues/19).
- [ ] **LMDB verification script** — Read compiled LMDB and print summary stats (entry counts per sub-db, tier boundaries, entity counts). Useful for CI and debugging.
- [ ] **Benchmark comparison** — Extend `run_benchmark.py` to compare results across runs (regression detection).

---

## Completed (recent)

- [x] compile_vocab_lmdb.py with morphological stripping (7f33603)
- [x] compile_entity_lmdb.py for fic/nf entity sub-dbs (7f33603)
- [x] merge_frequency_ranks.py with geometric mean blend (7f33603)
- [x] run_benchmark.py added (27151b6)
