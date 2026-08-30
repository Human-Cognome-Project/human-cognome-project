# Pass 4 — Data holdings (tracked files + live stores)

**2026-08-30. Dispositions per REBASE_REVIEW_PLAN.md; live probe read-only. Nothing executes until Pass 7.**
Cluster F: tracked holdings (`db/` 93, `data/` ~112, `sources/` 4, `source_doc_pbm/` 3). Cluster G: live probe of HAVEN (192.168.68.60:5435). Probe artifacts (rerunnable) in session scratchpad: `pass4_tables.txt`, `pass4_refcols.txt`, `pass4_roundtrip.py`, `pass4_roundtrip_results.txt`.

## Headline: the mash is smaller and more specific than feared

1. **The dotted structural convention held everywhere.** Zero structural violations in 1.49M hcp_english entries; every reference splits on dots into 2-character pairs, 1–5 deep. The precept was violated in *storage form* (TEXT blobs), not in *content*.
2. **The real defect is alphabet drift.** The canonical codec alphabet is 50 letters (52 minus O/o), but ~10.6% of hcp_english ids (157,804) — and traces in other shards (1,345 in hcp_core) — contain O/o: minted under a 52-letter alphabet before the exclusion, or by a non-codec minter. This is *the* per-table rule extraction needs.
3. **The arrayed form already exists in parallel.** The decomposed ns/p2–p5 columns re-join to `token_id` exactly for 99.996% of entries (62 mismatches, enumerable). **Extraction can pull the arrayed form directly from ns/p2–p5 and largely ignore the blobs.**
4. **Sentinels are flags, not parse failures:** `UNK:*` (hcp_core.pbm_entries), `BYTE_xx` (english_characters — the byte floor largely bypassed token addressing, consistent with the new byte-particle layer making it moot), `[literal]` bracket fallbacks (heavy in tokenized_etymology, ~40%), numeric dotted var-ids `00.00.00.00.NN` (fic_pbm docvars — a second, regular scheme).

## Live-store verdicts (cluster G)

| store | tables probed | rows (live) | address-column state | round-trip result | verdict |
|---|---|---|---|---|---|
| hcp_core | tokens, pbm_entries, lod_transitions | 6.27M tokens | dotted TEXT `token_id` PK + parallel ns/p2–p5 | tokens 1000/1000 clean; 1,345 O/o ids total; pbm_entries ~87% (rest `UNK:` sentinels) | **clean** (universal rule + tiny enumerable exceptions) |
| hcp_english | entries, relations, phrase_components, senses, english_characters, cx_formula | 1,494,216 entries (matches docs exactly) | dotted TEXT + ns/p2–p5 + native `text[]` for tokenized_* | structurally 100%; **10.6% O/o**; `[literal]` + `BYTE_xx` sentinels in arrays | **needs per-table rules** (alphabet-drift decode + sentinel flags) |
| hcp_fic_pbm | pbm_starters, pbm_documents | 83K starters, 9 docs | dotted TEXT; 3,206 numeric var-ids | ~87% codec-clean; var-ids a second regular scheme | **needs per-table rules** (var-id scheme + O/o) |
| hcp_fic/nf_people/places/things (6) | tokens, entity_names | 962–206K per shard | entity namespaces (uA/wA…) + ns/p2–p5 | 90–95% clean; failures O/o-only | **needs per-table rules** (O/o only) |
| hcp_envelope | 4 tables | tiny (6 defs) | n/a | n/a | **clean** — the schema itself is the pull (view-chain ancestor) |
| source_english | 15 tables | 1.44M entries, 9.8M sense_categories | integer surrogates + JSONB; no dotted addresses | n/a | **clean SRC** — raw+indexed side by side, path-through preserved |
| source_wiktionary | wiktextract_raw | 1.44M | raw JSONB | n/a | **clean SRC** — the raw face of the Kaikki singularity, intact |

**Discrepancies vs docs:** live reality = 10 hcp_* shards *plus* hcp_envelope, source_english, source_wiktionary, hcp_orchestrator (14 DBs); hcp_names confirmed eliminated (decision 002 executed); hcp_english count matches docs exactly; hcp_core 6.27M live vs 6.12M estimate; several nf-shard tables unanalyzed/empty; source_english.drain_progress empty (check at extraction time).

## Tracked holdings (cluster F)

| path/glob | disp | what it is | role in pull-driven extraction | notes |
|---|---|---|---|---|
| `db/*.sql.gz` + `.sha256` (12) | SRC | Sealed LFS dumps of six shards with checksums | Frozen fallback; extraction reads live HAVEN | Dump vintage predates later migrations — point-in-time only |
| `db/migrations/000–050` + shell helpers + README (~88) | AR (provenance) | The schema's full history — the old singularity's path-through record | 005*/047/048/049 document the precept's partial-enforcement history | **Duplicate numbers: 016×2, 038×2, 040×2, 041×2** — applied state, not filename order, is authoritative |
| `db/spacing_rules.sql` | SRC | Spacing-rule seed data (LFS) | Pull if new schema wants it | **Uncommitted "edit" is a no-op**: content sha256-matches the committed LFS oid (smudge artifact). Recommend discard-to-pointer; also check why it's LFS-tracked at all (`.gitattributes` covers only `db/*.gz`) |
| `db/load.sh` | CF | Checksum-verified dump loader | Enables local-load probing without touching HAVEN | Env-overridable |
| `db/tools/*.py` (3) | AR | One-shot seed generators | Outcomes live in hcp_core | — |
| `db/AGENTS.md` / `ROADMAP.md` / `TODO.md` | AR | DB-specialist meta | AGENTS.md schema-pattern section = concise decomposed-column reference | All stale (ROADMAP says 24 migrations; 51 exist) — corroborates the mash narrative; archive as-is |
| `data/gutenberg/` (110 texts + 2 metadata JSON) | SRC | Public-domain corpus + Gutendex metadata | First-wave documents; metadata = provenance records | Key future extraction on metadata IDs, not filenames |
| `sources/README.md` + `fetch.sh` | CF | Declared-source registry + fetcher | Registry pattern survives into new intake protocol | fetch.sh references machine-local `/usr/share/databases/reference/` |
| `sources/unicode_table.txt` | SRC | Unicode reference | Byte/codepoint layer | — |
| `sources/1780411991431.pdf` | SRC (flag) | Unidentified PDF | — | Identify before archive labelling |
| `source_doc_pbm/` (3) | AR | Feb-2026 infra design notes | SHARD-STATE snapshot = dated cross-check | Pure design history |

## Consolidated flags

1. **Patrick's ruling needed — O/o alphabet drift** (affects 157K+ ids and everything referencing them): remap O/o-bearing ids into canonical 50-letter space at extraction (re-mint = identity change, needs a mapping table), or admit a 52-letter legacy decode permanently. Extraction tooling supports either; the choice is doctrinal.
2. **Extraction strategy confirmed**: pull arrayed addresses from ns/p2–p5 directly; use blobs only as checksum. The 62 ns/p2–p5↔token_id mismatches and 1,345 hcp_core O/o ids are small enough to eyeball — lists reproducible from the probe queries.
3. **`db/spacing_rules.sql`**: discard the working-tree no-op to restore the pointer; investigate its LFS tracking rule before Pass 7.
4. **Migration numbering collisions** mean replay-order is unreliable; live schema state is the only authority (probe already works this way).
5. **Sentinel registry** (`UNK:*`, `BYTE_xx`, `[literal]`, numeric var-ids) goes into the new data-protocol doc as declared legacy conventions — flags on extraction, never parsed as addresses.
6. **The source_* DBs are model citizens**: raw JSONB kept beside the indexed form is exactly the path-through discipline the new protocol mandates — cite as internal precedent.
7. `hcp_orchestrator` is readable via the same hcp read path (rw creds failed earlier) — note for the queued graph review.
8. Unidentified `sources/1780411991431.pdf` needs identification (human or PDF read) before final labelling.
