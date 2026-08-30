# Pass 5 — Tooling and infra (`scripts/` 64, `tools/` 18, `.github/` 2; local-only dirs noted)

**2026-08-30. Dispositions per REBASE_REVIEW_PLAN.md. Nothing executes until Pass 7.**

**Totals: 1 CF group (5 files) · 1 CF→RW (2) · ~64 AR · ~12 DR (logs, CSV, committed binaries).**

| path/glob | disp | what it is | notes |
|---|---|---|---|
| `scripts/wiktionary/` (5: drain_english, load_lang, split_by_lang, scan_english_variants, source_english_schema.sql) | **CF** | The Kaikki/Wiktextract intake chain: split raw JSONL by language → COPY-load into source_wiktionary → drain into source_english (delta rule, idempotent) | Directly reusable for the landing-lattice intake; the drain's "every duplicating element becomes a row everyone points to" is single-source-of-truth compression in practice. Keep beside `src/hcp/db/kaikki.py` |
| `scripts/load_kaikki_fast.py`, `setup_kaikki_schema.py`, `migrate_to_proper_schema.py` | AR | Earlier kk_-era Kaikki schema/loaders (pre-source_* split); setup explicitly says "No raw JSON kept" — the discipline the wiktionary/ chain later fixed | Superseded twins; path-through record of the Kaikki singularity's earlier, lossier compression |
| One-shot minters/populators (~16 files: bootstrap_numerals*, fix_numeral_references, build_phrase_components, mint_*, create_*_entities, split_entity_dbs, merge_frequency_ranks, resolve_*, phrase_resolve_tokenized, self_tokenize, sweep_entity_glosses) | AR | Executed data-construction passes; outcomes live in the shards | Provenance for *how* the data got its shape. **`self_tokenize.py` records a destructive conversion** (original text fields cleared after tokenization) — archive's provenance section must cite it |
| `scripts/hcp_client.py`, `ingest_texts.py`, `run_benchmark.py` | AR | Socket clients for the retired port-9720 daemon | Dead with the engine |
| `scripts/deprecated/` (31) | AR; logs/CSV **DR** | Self-labelled deprecated passes + run logs + review wordlists | The 8 `.log` + 1 `.csv` are generated run detritus (DR); `.py` files archive as provenance |
| `scripts/context_watcher.sh` | AR | tmux watcher for multi-instance Claude coordination | Era relic |
| `scripts/create_github_issues.sh` | AR | Feb-2026 issue seeding script | Maps issue numbers to intent — input for Pass 7 closure comments |
| `scripts/TODO.md` | AR | Stale script inventory | References `compile_vocab_lmdb.py` which no longer exists — stale pointer, corroborates drift |
| `tools/byte-floor/` (7) | AR | Standalone C++ byte→character resolver ("THE SEAM" reference) | Doctrine captured in Pass 3; archive intact |
| `tools/gloss-kernel/` (7) | AR; binary **DR** | C++ fixpoint gloss→concept-formula ladder | Fixpoint laddering (known concepts mint new until closure) = transitive-chain/accretion-order precedent — cite in new intake doc. Committed ELF executable is build output: DR |
| `tools/foma/` (3) | AR; `.bin` **DR** | Foma morphological analyzer PoC | `.bin` generated from `.foma`/`.lexc`: DR |
| `tools/hcp.py` | AR | Asset-manager CLI for retired daemon | Dead with the socket API |
| `.github/ISSUE_TEMPLATE/` (2) | **CF→RW** | agent-suitable + good-first-issue templates | Structure and label taxonomy survive; wording light-rewritten in Pass 7 |
| *(local-only, gitignored)* `infra/`, `resources/`, `relay/`, `benchmarks/`, caches | — | relay/ is live Discord infra and stays out of the rebase; benchmarks/ outputs dead with engine; rest machine-local | Not review objects |

## Flags

1. **Dead-twin question (from Pass 3) answered**: real twins are within `scripts/` — the kk_-era Kaikki loaders vs the `scripts/wiktionary/` chain; the latter survives as the only scripts CF. No live duplication against `src/hcp/ingest/`. LMDB compilation exists *nowhere* anymore (`compile_vocab_lmdb.py` referenced by two meta files but gone from tree) — record in archive honesty notes.
2. **Committed build artifacts** (`tools/gloss-kernel/gloss-kernel` ELF, `tools/foma/english_morph.bin`) and `scripts/deprecated/` run logs/CSV: DR — the only deletions this pass proposes, all regenerable or detritus.
3. **`self_tokenize.py`** must be cited wherever the archive documents data provenance: original text was deliberately consumed; nobody should hunt for it later.
4. `.github/` has no workflows — CI never landed (confirms #43); nothing to migrate.
