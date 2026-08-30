# Extraction toolkit

The read-only pull toolkit for building the new substrate from the previous era's stores. Carried
forward from `src/hcp/` and `scripts/wiktionary/` at the 2026-08 rebase (provenance:
[/review/pass-03-code.md](../review/pass-03-code.md), [pass-05](../review/pass-05-tooling.md));
originals preserved under [/archive/2026-08-rebase/](../archive/2026-08-rebase/).

## Contents

- **`token_id.py`** + **`tests/test_token_id.py`** — the arrayed-pair address codec: dotted display
  form ↔ integer-pair tuples, 1–5 pairs, base-50 alphabet (52 letters minus O/o), LoD depth. This is
  the executable statement of the addressing precept.
- **`postgres.py`, `english.py`, `names.py`** — shard connectors (hcp_core, hcp_english; names is
  historical). **Connection defaults in code are stale** — the live stores are on HAVEN per
  [../docs/legacy-data-maps/database-access.md](../docs/legacy-data-maps/database-access.md).
- **`kaikki.py`** + **`wiktionary/`** — the Kaikki intake chain (split raw JSONL by language →
  load `source_wiktionary` → drain to `source_english`; idempotent, raw preserved beside indexed).
- **`gutenberg_fetch.py`** — Gutendex fetcher with provenance metadata.

## Rules of use

1. **Read-only against the old stores.** Extraction pulls; nothing repairs in place.
2. **O/o correction**: ids containing O/o are remapped into the canonical 50-letter space at
   extraction, with the mapping table kept ([/review/decisions.md](../review/decisions.md) §4).
   The codec's strict 50-letter alphabet is correct; a 52-letter *legacy decode* is used only to
   read old ids for remapping.
3. **Sentinels are flags, not addresses**: `UNK:*`, `BYTE_xx`, `[literal]`, numeric var-ids.
4. Pull arrayed addresses from the decomposed `ns/p2–p5` columns; use the blob as checksum
   ([/review/pass-04-data.md](../review/pass-04-data.md)).
5. These modules were lifted from a package (`hcp.*`) — internal imports may need a path shim until
   the toolkit gets its own packaging.

Python is legitimate here: this is front-end pull tooling, not the hot path.
