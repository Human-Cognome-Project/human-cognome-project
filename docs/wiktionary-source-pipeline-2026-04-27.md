# Wiktionary → source_english pipeline (2026-04-27)

Built a fresh source-side ingest of the full raw Wiktextract dump (Kaikki),
producing two new Postgres databases on the NAS:

- **`source_wiktionary`** — raw archive of Wiktextract entries as JSONB. One row per upstream entry, fully preserved.
- **`source_english`** — structured/dedup'd English subset, drained from `source_wiktionary` via the **delta rule**: any element that duplicates across entries (tag, category, gloss-path-prefix, etymology blob) is minted as its own row that everyone points to.

This replaces the old ingestion path (the `hcp_english` shard built earlier).
The old shard stays intact; this work is *upstream of it*. Eventual transfer
into the proper `hcp_english` shard will reuse existing token_IDs where
applicable; new entries get freshly minted IDs at transfer time.

## Why source-side compression first

The raw Wiktextract dump is ~10.5M entries / 2.4 GB gzipped / ~20 GB
uncompressed JSONL. Walking it for any analytical question (find regional
variants, find shared gloss subtrees, find category usage) means re-streaming
the file. That makes per-entry LLM review (planned next stage with
Ministral/Gemma) impractical.

By pre-draining English into a structured Postgres DB:

- Per-entry queries become indexed lookups
- Cross-entry dedup is inspectable (e.g., "A surname." root shared by 28,984 entries; "A number of places in the United States:" parent has 6,258 children)
- The `notes` columns on `entries`, `categories`, `lex_relations` etc. become triage scratchpads during analysis passes
- Drainage flag preserves what's been processed; undrained = the validation signal

## Pipeline

```
raw-wiktextract-data.jsonl.gz  (Kaikki, 2.4 GB)
        │
        ▼  scripts/wiktionary/split_by_lang.py
4,688 per-language jsonl files at /volume2/hcp/staging/by_lang/
        │   (10,530,584 total entries, zero parse errors)
        ▼  scripts/wiktionary/load_lang.py
source_wiktionary.wiktextract_raw  (raw JSONB, 1 row per entry)
        │   (English subset: 1,454,988 rows; 3.4 GB)
        ▼  scripts/wiktionary/drain_english.py
source_english  (structured, dedup'd, 2.6 GB on disk; 258 MB compressed dump)
```

## Storage locations (NAS)

| Path | Purpose | Size |
|---|---|---|
| `/volume2/hcp/staging/by_lang/*.jsonl` | per-language splits, English purged | 18.3 GB (4,687 files) |
| `/volume2/hcp/dumps/source_english.dump` | canonical compressed archive of source_english | 258 MB |
| Postgres `source_wiktionary` (port 5435) | raw JSONB archive (English only currently) | 3.4 GB |
| Postgres `source_english` (port 5435) | structured English source | 2.6 GB |

The original gz file was deleted (preserved in source_wiktionary as JSONB).
The English jsonl was deleted (preserved in source_wiktionary AND source_english).
Other-language jsonls remain on the NAS for future loads.

## source_english schema

15 tables. IDs are throwaway bigserials; matching to `hcp_english` token_IDs
happens at the future transfer stage, not here.

### Root entry tables

- **`entries`** — one row per (word, pos, etym_number) Wiktextract entry. `source_id` is the natural key (matches `wiktextract_raw.id`). Note: (word, pos, etym_number) is **not** unique in source — Wiktextract genuinely produces multiple entries per natural key (e.g., `wares` has both the lemma and the inflected-form entry).
- **`etymology_blobs`** — dedup'd by sha256 hash on text. ~10% prose dedup observed.

### Sense tree

- **`senses`** — leaves of the gloss tree, linked back to entries.
- **`gloss_nodes`** — the dedup'd hierarchy. Each unique `(parent_id, text)` is one row. UNIQUE on `(COALESCE(parent_id, 0), text)`. Senses point at their leaf gloss_id; full path is recovered by walking parent_id upward.

### Vocab pivots (M:M throughout)

- **`tags`** ↔ `sense_tags`, `form_tags`, `sound_tags`. Has self-FK `canonical_id` for future canonicalization (e.g., `British` → `UK`, not yet populated).
- **`categories`** ↔ `sense_categories`. Has `notes` triage column.

### Per-entry attached structures

- **`forms`** ↔ `form_tags` — inflections, alternate spellings, regional forms
- **`examples`** — example sentences and quotes per sense
- **`lex_relations`** — synonyms, antonyms, hyponyms, hypernyms, meronyms, holonyms, coordinate_terms, related, derived (consolidated with `relation_type` column)
- **`sounds`** ↔ `sound_tags` — IPA, enPR, audio URLs

### Misc

- **`drain_progress`** — singleton row tracking start/completion
- All M:M joining tables have explicit `notes text` triage column where useful

### Storage policies

- `notes` columns are scratchpads for working triage (e.g., "this category is a literary reference, investigate"). Not for long-term metadata.
- Ellipsis-fragmented gloss arrays (e.g., the article entry for "the" splits one sentence across multiple gloss-array elements with `...` bridges) are preserved as-is. This is editorial structure from Wiktionary, not noise.

## Drainage state

Full English subset drained: **1,454,988 / 1,454,988** with **0 errors** and **0 undrained**. Drain ran in 2.2 hours after batched-commit optimization (~176 rows/sec steady state on the N5105).

| Cardinality | Count | Notes |
|---|---|---|
| entries | 1,454,988 | |
| senses | 1,739,532 | avg 1.20 per entry |
| gloss_nodes | 1,575,002 | the dedup'd tree |
| tags | **664** | small finite vocab — extreme dedup |
| categories | 39,132 | |
| sense_tags (M:M) | 2,586,980 | |
| sense_categories (M:M) | 9,908,490 | dense |
| forms | 896,243 | |
| form_tags (M:M) | 1,166,434 | |
| examples | 730,229 | |
| lex_relations | 1,840,659 | |
| sounds | 434,424 | |
| sound_tags (M:M) | 197,614 | |
| etymology_blobs | 470,305 | ~10% prose dedup |

### Where dedup pays out heaviest

- `"A surname."` root → **28,984 entries**
- `"A surname from German."` → 3,524
- `"A number of places in the United States:"` parent has **6,258 direct children** (specific places branch off)
- `"A placename:"` parent has 1,628 direct children

### Where it doesn't (uneven structural compression)

- `"Terms relating to animals."` is only used by `cat`. Wiktionary editors didn't standardize this root the way they did for surnames/placenames.
- 1,520,859 of 1,529,316 root nodes are leaf-only (single-element glosses array). Most senses are unique at top level; multi-level senses are the minority.

## Architectural rules captured

- **Drain from source, don't convert in place**: the source DB is preserved as authoritative reference; structured target is derivable from it. Undrained remainder is the validation signal.
- **Delta rule = structure**: anything that duplicates becomes its own row (a pivot point); anything unique stays inline.
- **M:M is the standard relationship**: specifics surface from cross-joins, never lookups.
- **IDs in source_english are throwaway**: matching to `hcp_english` token_IDs happens at transfer time. Scratch identity here.
- **Don't repurpose state-tracking columns**: per-stage drainage flags, not a generic `drained` boolean. (See `memory/feedback_pipeline_state_columns.md`.)
- **Preserve source structural choices**: don't normalize away patterns like ellipsis-fragmented glosses; Wiki structured them that way deliberately.

## Next stage (planned)

LLM-driven per-entry review (Ministral or Gemma) using `source_english` as the
queryable substrate. Goals:

1. Distinguish **real categories** (literary references, periods, registers) from **derived labels** (things substitutable at query time by walking the explication tree, e.g., "is an animal" on `cat`)
2. Tighten gloss specificity for compound entries (e.g., `Persian cat` should anchor to `Felis catus`, not loose prose linking to `cat` headword)
3. Build NSM explication trees per entry — the substrate for cross-entry semantic computation later

The drain produced the queryable substrate that makes per-entry LLM review tractable.

## Scripts

All in `scripts/wiktionary/`:

| Script | Role |
|---|---|
| `split_by_lang.py` | streaming JSONL split by `lang_code` into per-language files |
| `load_lang.py` | COPY-load a per-language JSONL into `source_wiktionary.wiktextract_raw` |
| `drain_english.py` | drain English subset from `source_wiktionary` to `source_english` |
| `scan_english_variants.py` | enumerate English-family variants by Wiktextract `lang` field |
| `source_english_schema.sql` | full schema for `source_english` |

These are operational drivers, not engine code (Python is front-end feed only;
all engine pipeline logic stays in C++).
