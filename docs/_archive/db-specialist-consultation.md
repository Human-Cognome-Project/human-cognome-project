# DB Specialist Consultation — Engine Storage Architecture

**From**: Engine Specialist
**Re**: Storage architecture review, pending migrations, PBM revision, modular Gem direction
**Date**: 2026-03-01

---

## 1. Current Engine-Side DB Architecture

The engine touches Postgres through **four separate modules**. Patrick's direction: **small, composable kernel modules** — each does one thing, callable from many places, chainable into task-specific flows. Not monolithic gems that do one complex thing for themselves only.

**Module inventory:**

| Module | File | Postgres DBs touched | Purpose |
|--------|------|---------------------|---------|
| HCPWriteKernel | `HCPStorage.h/cpp` | hcp_fic_pbm, hcp_fic_entities, hcp_nf_entities | PBM bond storage, position storage, metadata, provenance, entity cross-refs, document listing |
| HCPCacheMissResolver | `HCPCacheMissResolver.h/cpp` | hcp_english, hcp_core, hcp_var | Handler-per-sub-db LMDB backfill from Postgres on cache miss |
| HCPEnvelopeManager | `HCPEnvelopeManager.h/cpp` | hcp_core + any shard via stored queries | Envelope definition loading, kernel chain execution into LMDB, manifest tracking, eviction |
| HCPBondCompiler | `HCPBondCompiler.h/cpp` | hcp_english, hcp_core | Char-word and byte-char bond table compilation (startup, cached in hcp_temp) |

**The problem**: `HCPWriteKernel` has grown into a monolith. It does PBM storage, position storage, metadata, provenance, entity cross-refs, document detail queries, bond queries, docvar queries, and var classification all in one class.

**Proposed decomposition** (engine will implement, for your awareness):
- **StorePBM kernel** — bond insertion only (documents + starters + bond subtables)
- **StorePositions kernel** — position + modifier encoding onto starters
- **Metadata kernel** — JSONB merge, provenance, document detail
- **DocVar kernel** — var classification, grouping, extended queries
- **EntityQuery kernel** — cross-DB entity lookups (fiction entities, NF authors)
- **DocumentQuery kernel** — listing, detail, bond browsing

Each becomes its own small Gem source file, composable into flows.

---

## 2. Migration 020: Positional Modifiers

**File**: `db/migrations/020_position_modifiers.sql`
**Target DB**: hcp_fic_pbm
**Status**: Written, not yet applied to dumps

**What it does**: Adds `modifiers TEXT` column to `pbm_starters`.

**Why**: Morph bits (inflection) and cap flags (capitalization) are stored as **positional modifiers** — sparse deltas on the position map. Design principle (Patrick): store deltas, not copies. Bare tokens (lowercase uninflected) carry zero overhead. Only tokens with something to say (an inflection, a cap flag) get modifier entries.

**Encoding format**: Sparse base-50 pairs concatenated into a single TEXT value:
```
[position_b50(4 chars) + modifier_b50(4 chars)] repeated for each non-zero modifier
```

**Modifier value packing**: `(morph_bits << 2) | cap_flags`
- `cap_flags`: bit 0 = first_cap, bit 1 = all_caps (2 bits)
- `morph_bits`: 16-bit field — PLURAL(0), POSS(1), POSS_PL(2), PAST(3), PROG(4), 3RD(5), NEG(6), COND(7), WILL(8), HAVE(9), BE(10), AM(11), bits 12-15 reserved

**Engine implementation**: `EncodeSparseModifiers()` in HCPStorage.cpp writes them during `StorePositions()`. `LoadPositions()` reads them back via a sparse `posModMap` lookup during position decoding.

**Questions**:
1. Is a single TEXT column the right approach, or should modifiers be a separate table (position → modifier) for queryability? The TEXT blob is compact and fast for the engine's sequential decode, but opaque to SQL queries.
2. The base-50 encoding reuses EncodePosition/DecodePosition. Max modifier value under current packing: `(0xFFFF << 2) | 0x3 = 262143`, fits in base-50 4-char encoding (max 6,249,999). Future-proof enough?
3. Should we add a `has_modifiers BOOLEAN` generated column for fast filtering?

---

## 3. Migration 021: Activity Envelopes

**File**: `db/migrations/021_activity_envelopes.sql`
**Target DB**: hcp_core
**Status**: Written, not yet applied

**What it does**: Creates the envelope schema — 4 tables in hcp_core:

| Table | Purpose |
|-------|---------|
| `envelope_definitions` | Named envelopes with active flag |
| `envelope_queries` | Stored SQL queries per envelope, each targeting a shard DB and LMDB sub-db |
| `envelope_includes` | Composition — parent includes child's queries |
| `envelope_activations` | Audit log of activations with load stats |

**Bootstrap examples included**:
- `english_common_10k` — top 10K freq-ranked words across word lengths 2-16, one query per length bucket targeting `vbed_XX` sub-dbs
- `fiction_victorian` — composes english_common_10k + archaic/literary/formal category terms

**Engine implementation** (already built — `HCPEnvelopeManager`):
- `LoadEnvelope()` — reads definition + queries + children from Postgres
- `ActivateEnvelope()` — evicts previous → loads children depth-first → executes own queries → records manifest → logs audit
- `ExecuteQuery()` — runs stored SQL on target shard → writes result rows (col0=key, col1=value) to target LMDB sub-db
- `EvictManifest()` — reads `_manifest` LMDB sub-db → drops tracked sub-dbs
- `Prefetch()` — executes first N queries ahead of current progress
- `GetShardConnection()` — lazy-opened per-shard Postgres pool

**Questions**:
1. **Query format contract**: Currently assumes all envelope queries return exactly 2 columns (key, value) for direct LMDB insertion. Support multi-column results with a format specifier? Or keep strict 2-column?
2. **Parameterized queries**: `envelope_queries.query_text` stores raw SQL with `format()` expansion. `param_types TEXT[]` column exists but is unused — engine calls `PQexec()` not `PQexecParams()`. Wire parameterization, or keep self-contained?
3. **Manifest granularity**: Current eviction drops entire LMDB sub-dbs (`mdb_drop`). Activating a new envelope wipes ALL entries in a sub-db, even if another envelope also wrote to it. Track individual keys instead?
4. **Envelope versioning**: No version/checksum on queries. If a query is updated in Postgres, cached LMDB data becomes stale. Add `version INTEGER` or `checksum TEXT`?
5. **Cross-shard consistency**: Queries can target multiple Postgres DBs with no transactional guarantee across shards. Acceptable, or add two-phase approach?
6. **LMDB sub-db conflict**: Bootstrap examples write to `vbed_XX` sub-dbs — same ones BedManager uses for pre-compiled vocab. Envelope activation could overwrite pre-compiled data. Separate sub-dbs (e.g., `env_vbed_05`)? Or is the intent that envelopes replace static data?

---

## 4. PBM Generation — Upcoming Revision

Patrick has directed a PBM generation revision.

**Current flow** (CPU algorithmic):
```
Tokenize(text) → TokenStream → DerivePBM(stream) → StorePBM()
```

`DerivePBM()` is a CPU-side pair walk: iterate adjacent tokens, count pairs, build `PBMData.bonds[]`. It knows nothing about morphology, capitalization, or tier.

**New direction**: The particle-stream resolution pipeline (BedManager/VocabBed) already resolves words via PBD simulation on GPU. Adjacent particle positions after settlement ARE bond adjacency. PBM bonds can be derived directly from physics output rather than a flat sequential walk.

**What this means for DB**:
- Bond data format may change — the physics pipeline produces `ResolutionResult` structs with `{word, tokenId, tier, morphBits, capFlags}` per resolved run
- Patrick specified: "Positional modifiers need to be noted in the PBM notation" — morph bits and cap flags may need to appear in bond data, not just the positional map
- Current bond subtables only store `(starter_id, b_side_parts, count)` — no modifier columns

**Questions**:
1. **Modifiers in bonds**: If "walked→the" becomes base-form "walk→the" with PAST modifier on the A-side, where does the modifier live?
   - **(a)** Modifier column on bond subtable row: `(starter_id, b_p3, b_p4, b_p5, count, a_modifier)`
   - **(b)** Inherited from the starter's positional modifier data (no bond table change)
   - **(c)** Bonds are always between base forms; modifier data lives exclusively in the position map. Bonds compact inflections (all forms of "walk" = one bond entry, stronger count), positions preserve surface form.
   - Patrick's prior statement: "All inflections of 'walk' collapse to one base token with positional modifiers. Deeper bond stacking, stronger signal, smaller data." — strongly suggests **(c)**.
2. **Bond count semantics**: If "walked", "walking", "walks" all collapse to "walk" in bonds, the count for "walk→the" increases. Does the DB schema need any change, or is the count column sufficient?
3. **Bond derivation location**: Physics pipeline produces `ResolutionResult` per run. Should bond derivation happen engine-side (StorePBM receives the same `PBMData` format), or should we send richer data to Postgres?

---

## 5. Current Position + Bond Storage Data Flow

### Write path
```
Tokenize(text, vocab)
  → TokenStream { tokenIds[], positions[], totalSlots, modifiers[] }

DerivePBM(TokenStream)
  → PBMData { bonds[], firstFpbA, firstFpbB, totalPairs, uniqueTokens }

StorePBM(name, century, pbmData)
  → INSERT pbm_documents (ns, p2, p3, p4, p5, name, first_fpb_a, first_fpb_b, total_pairs)
  → For each bond.tokenA: INSERT pbm_starters (doc_id, a_ns, a_p2, a_p3, a_p4, a_p5)
  → Route each bond to subtable by B-side namespace:
      AB.AB.* → pbm_word_bonds (starter_id, b_p3, b_p4, b_p5, count)
      AA.*    → pbm_char_bonds (starter_id, b_p2, b_p3, b_p4, b_p5, count)
      AA.AE.* → pbm_marker_bonds (starter_id, b_p3, b_p4, count)
      var.*   → pbm_var_bonds (starter_id, b_var_id, count)

StorePositions(docPk, tokenIds, positions, totalSlots, modifiers)
  → UPDATE pbm_documents SET total_slots, unique_tokens
  → For each unique token: encode positions as base-50 packed TEXT
  → For tokens with non-zero modifiers: encode sparse [pos+mod] pairs
  → UPDATE/INSERT pbm_starters SET positions, modifiers
  → Var tokens: "AA.AE.AF.AA.AC surface" → lookup pbm_docvars → "00.00.00.XX.YY"
```

### Read path
```
LoadPositions(docId)
  → SELECT token_a_id, positions, modifiers FROM pbm_starters
      WHERE doc_id = ? AND positions IS NOT NULL
  → Decode base-50 positions (4 chars each)
  → Decode sparse modifiers (8 chars each: [pos_b50 + mod_b50])
  → Resolve var starters: "00.00.00.XX.YY" → lookup pbm_docvars → "AA.AE.AF.AA.AC surface"
  → Sort by position → ordered token IDs

LoadPBM(docId)
  → SELECT from pbm_documents (header: firstFpbA, firstFpbB)
  → Load all starters, resolve var-encoded A-sides
  → 4 separate bond queries (word, char, marker, var), reconstruct full B-side token IDs
  → Return PBMData { bonds[], firstFpbA, firstFpbB, totalPairs, uniqueTokens }
```

---

## 6. LMDB Sub-Database Layout

| Sub-db | Key | Value | Module |
|--------|-----|-------|--------|
| w2t | word form (UTF-8) | token_id (UTF-8) | CacheMissResolver (WordHandler) |
| c2t | 4-byte codepoint | token_id (UTF-8) | CacheMissResolver (CharHandler) |
| l2t | label form (UTF-8) | token_id (UTF-8) | CacheMissResolver (LabelHandler) |
| t2w | token_id (UTF-8) | word form (UTF-8) | Vocabulary reverse lookup |
| t2c | token_id (UTF-8) | 4-byte codepoint | Vocabulary reverse lookup |
| forward | prefix key | "0"/"1"/token_id | Boilerplate prefix check (being revised) |
| vbed_02..vbed_16 | "data" / "meta" | packed binary / VBedMeta struct | BedManager (pre-compiled vocab) |
| _manifest | "env:name" | newline-delimited sub-db names | EnvelopeManager eviction tracking |

**Questions**:
1. vbed sub-dbs use binary format; all others use plain UTF-8. Acceptable inconsistency, or standardize?
2. `forward` sub-db is being revised (boilerplate push on cache miss replaces forward walk). Remove from contract?
3. Concerns about `_manifest` text-based approach for eviction tracking?

---

## 7. Docvar Classification & Storage

Engine-side `ClassifyVar()` (HCPStorage.cpp) detection order:
1. **uri_metadata**: starts with `http://`, `https://`, `www.`, or contains `://`
2. **sic**: contains digits or non-alpha/non-space/non-hyphen/non-apostrophe chars
3. **proper**: first character uppercase
4. **lingo**: everything else

Stored in `pbm_docvars.var_category`. Grouping goes to `docvar_groups` (migration 015).

**Question**: Is this classification logic correct? Detection order matters — URI check before sic prevents URLs from being classified as sic.

---

## 8. Modular Gem Direction

Patrick's directive: **smaller modules that we can kernelize the flow for specific tasks, rather than complex gems that do one thing for themselves only**.

This applies to both engine Gems and DB-side stored procedures:
- Each module = one kernel operation (StorePBM, StorePositions, EnvelopeActivate, etc.)
- Flows = chained kernel calls in a specific order for a specific task
- Same kernel callable from multiple flows (socket API, console command, envelope activation, future NAPIER policy)

For DB: stored procedures/functions should follow the same pattern. Envelope activation = chained prepared statements. PBM storage = chained inserts. Each step is a discrete, testable unit.

---

## 9. Consolidated Questions

**Migration 020 (modifiers)**:
1. Single TEXT column vs separate modifier table?
2. Base-50 encoding range sufficient for future modifier bits?
3. Add `has_modifiers` generated column for filtering?

**Migration 021 (envelopes)**:
4. Support multi-column query results, or strict 2-column (key, value)?
5. Wire parameterized queries, or keep self-contained SQL?
6. Manifest: track sub-dbs (current) or individual keys?
7. Add query versioning/checksums for staleness detection?
8. Cross-shard consistency approach?
9. Envelope writes vs pre-compiled vocab — same LMDB sub-dbs or separate?

**PBM revision**:
10. Where do modifiers live in bond data — option (a), (b), or (c)?
11. Bond count semantics with inflection collapsing — schema change needed?
12. Bond derivation: engine-side or DB-side?

**LMDB layout**:
13. Binary vbed sub-dbs vs UTF-8 others — acceptable?
14. Remove `forward` sub-db from contract?
15. Manifest approach for eviction — concerns?

**Docvars**:
16. Classification logic correct?

**General**:
17. Column naming conflict (engine noticed 2 cols same data, different names) — still outstanding from previous session.
