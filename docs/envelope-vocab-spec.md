# Envelope Vocab Spec

**Updated:** 2026-03-10
**Status:** All blockers resolved (migrations 025–028 applied). This document reflects the current state.

---

## What the engine does

1. Client calls `source_activate_envelope <name>` on the socket
2. `HCPEnvelopeManager::ActivateEnvelope` reads the envelope definition and its queries from `hcp_core`
3. For each query: connects to the target shard DB (`shard_db` field), runs the SQL, writes `col[0] → col[1]` to the target LMDB sub-db
4. `BedManager::RebuildVocab()` cursor-scans LMDB `env_vocab`, builds in-memory vocab index per word length
5. `BedManager::Resolve` runs CharRuns through PBD against that vocab

---

## LMDB sub-db ownership

Three separate sub-dbs. Do not mix them.

| Sub-db | Owner | Lifecycle | Purpose |
|--------|-------|-----------|---------|
| `env_vocab` | `EnvelopeManager` | Evicted on envelope switch | Envelope-loaded vocab (key = lowercase word, value = token_id) |
| `w2t` | `CacheMissResolver` | Survives envelope switch | Runtime cache-miss fills (word → token_id) |
| `vbed_*` | `BedManager` | Never touched by envelopes | Pre-compiled binary vocab beds |

All envelope queries write to **`env_vocab`**. The `w2t` sub-db is filled independently by the cache-miss resolver and is not evicted when the envelope switches — this preserves resolver work across context changes.

---

## Envelope composition

`fiction_victorian` includes `english_common_10k` via `envelope_includes`.

The engine resolves **one level of nesting only** — it loads child queries but does not recurse into grandchildren. The current two-level nesting is within that limit.

---

## Queries: `english_common_10k`

16 queries, priorities 1–16. All target `lmdb_subdb = 'env_vocab'`.

### Priority 1: Label tier (tier 0 broadphase)

```sql
SELECT lower(name) AS name, token_id
FROM tokens
WHERE proper_common = 'proper'
  AND canonical_id IS NULL
  AND length(name) BETWEEN 2 AND 16
ORDER BY freq_rank ASC NULLS LAST
```

**`AND canonical_id IS NULL` is required.** Variant entries (uppercase collision forms) point to a lowercase canonical via `canonical_id`. Loading a variant's `token_id` into `env_vocab` would produce wrong lookups — the engine lowercases the input before lookup and expects to find the canonical. Only canonical entries (no `canonical_id`) are loaded.

### Priorities 2–16: Freq-ranked common vocab (per word length)

```sql
-- Priority 2 (length 2):
SELECT lower(name) AS name, token_id
FROM tokens
WHERE length(name) = 2 AND freq_rank IS NOT NULL
ORDER BY freq_rank ASC LIMIT 1500

-- Priorities 3–16: same pattern with length = N, LIMIT 1500 (short) or 500 (long)
```

No `canonical_id IS NULL` guard needed here — these are base-form vocab entries, not proper nouns with collision variants.

---

## Queries: `fiction_victorian`

Inherits all 16 `english_common_10k` queries via `envelope_includes`. Adds one extra query at priority 1 (or lowest priority — check `envelope_queries` table):

```sql
SELECT lower(name) AS name, token_id
FROM tokens
WHERE category IN ('archaic', 'literary', 'formal', 'dialect')
ORDER BY freq_rank ASC NULLS LAST
```

`formal` category is not currently populated (0 rows) but is kept for forward compatibility. Valid categories in the data: `archaic` (4,579), `dialect` (143), `casual` (74), `literary` (19).

---

## Proper noun system (`proper_common` column)

Labels (proper nouns) are stored **lowercase** in `name`, just like all other tokens. The `proper_common = 'proper'` flag is the capitalize-always marker.

### How tokens are tagged

Migration 026 seeded the initial set: `layer = 'C'` (Wiktionary "Capitalised" category) + `subcategory = 'label'` → 141,756 entries tagged.

Migration 028 extended tagging to collision-resolution canonicals: lowercase tokens that are the canonical form of an uppercase collision entry also receive `proper_common = 'proper'`.

**`name != lower(name)` is NOT a Label indicator.** Uppercase in `name` is a normalization bug, not a proper noun marker.

### Collision variant system

When an uppercase entry (e.g. `"Apaches"`) has a lowercase counterpart (`"apaches"`):
- The uppercase entry is a **variant**: `canonical_id → lowercase token_id`, `proper_common = 'proper'`
- The lowercase entry is the **canonical**: `proper_common = 'proper'`, `canonical_id IS NULL`

The engine lowercases input before lookup, so it finds the canonical directly. The variant entry exists to carry the `proper_common` tag for reconstruction, not for direct vocab loading.

58,784 collision pairs wired (migration 028). Deferred:
- **438 cases**: uppercase entry with 2+ lowercase counterparts — ambiguous canonical, pending Patrick's input
- **1 case**: Gorky (`AB.AB.CA.HX.ZD` vs `gorky AB.AB.CC.AS.li`) — held
- **45,444 multi-role name groups**: same surface form, different linguistic role (e.g. `"Ah"` as affix/prefix AND as word/noun) — these are NOT import artifacts; they legitimately coexist. Pending Patrick's guidance on canonical designation.

---

## Inflected forms

Envelope queries load **base forms only** (freq_rank-ranked canonical vocab). The engine strips inflections before lookup — plurals, past tense, progressives, etc. reduce to their base before hitting `env_vocab`. Loading inflected forms would waste LIMIT budget without benefit.

Future: an `is_base_form` flag on `hcp_english.tokens` would allow `WHERE is_base_form = true` to exclude inflected forms that accidentally carry a `freq_rank` (they currently consume LIMIT slots). Not a blocker.

---

## Shard connections

`HCPEnvelopeManager::GetShardConnection` currently hardcodes `host=localhost dbname=<shardDb> user=hcp password=hcp_dev`. The `shard_registry` table in `hcp_core` exists but is not used for connection string lookup. For single-host deployments this is fine. Multi-host support requires wiring `shard_registry.conn_str` into `GetShardConnection`.

---

## Schema reference (`hcp_core`)

```
envelope_definitions  — name, description
envelope_queries      — envelope_id, shard_db, query_text, priority, lmdb_subdb, description
envelope_includes     — parent_id, child_id  (1-level nesting; engine does not recurse deeper)
envelope_activations  — envelope_id, activated_at, deactivated_at
```

---

## Migration history

| Migration | What |
|-----------|------|
| 021 | Created envelope schema + bootstrap data (bugs present) |
| 025 | Fixed 3 bugs: `word`→`name`, `frequency_rank`→`freq_rank`, `env_w_XX`→`env_vocab` |
| 026 | Populated `proper_common='proper'` for 141,756 Wiktionary Label-tier tokens; added priority-1 query to `english_common_10k` |
| 027 | Lowercased 141,755 proper noun names (1 collision excluded) |
| 028 | Wired 58,784 uppercase collision entries as variants; added `AND canonical_id IS NULL` to Label tier query |
