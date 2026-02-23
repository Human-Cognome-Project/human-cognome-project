# PBM Bond Storage — DB Specialist Requirements

**Date:** 2026-02-20
**From:** Engine specialist
**Context:** Physics detection scene needs PBM bond data at sub-word LoD levels as force constants. Patrick confirmed these should be stored (not just computed in memory).

## What We Need Stored

### 1. Byte → Char PBM Bonds (core, AA namespace)

**Data:** (byte_a, byte_b) → count
- byte_a, byte_b: values 0-255
- count: how many known characters contain this byte pair adjacently in their encoding
- Example: UTF-8 'é' = bytes [0xC3, 0xA9] contributes one count to (0xC3, 0xA9)

**Characteristics:**
- Lives in core (encoding patterns are universal, not language-specific)
- 256² = 65,536 max possible pairs, but very sparse (probably a few hundred non-zero for UTF-8)
- Static — only changes if new encoding support is added
- Small total storage

**Engine query patterns:**
- Bulk read at startup: load entire table into memory
- Point query during simulation: bond strength between two byte values

### 2. Char → Word PBM Bonds (language shard, e.g. AB namespace for English)

**Data:** (char_a, char_b) → count
- char_a, char_b: character values (or character token IDs — your call on key format)
- count: how many known words in this language contain this character pair adjacently
- Example: "the" contributes counts to (t,h) and (h,e)

**Characteristics:**
- Lives in language shard (letter ordering patterns are language-specific)
- English: ~26 lowercase + 26 uppercase + digits + accented = maybe 70 characters
- 70² ≈ 5,000 max possible pairs, probably 1,000-1,500 non-zero
- Updates when vocabulary grows (new words = new or incremented letter-level bonds)
- Small per language

**Engine query patterns:**
- Bulk read at startup: load entire table into memory
- Point query during simulation: bond strength between two characters

### 3. Word → Sequence PBM Bonds (per-document, language shard)

**Data:** (token_id_a, token_id_b) → count, per document
- This IS the stored document (stacked dumbbells)
- Replaces position-based document storage
- token_id_a, token_id_b: vocabulary token IDs (hierarchical names like AB.AC.AD)
- count: how many times this bond appears in the document (stack height)

**Characteristics:**
- Per-document storage — each document is a set of bond (pair, count) entries
- Slightly more data than positional encoding per document
- But: faster to reassemble (no position→PBM conversion), saves conversion before aggregation
- PBMs can be added or removed from inference token by token
- Immutable after ingest (edits create deltas, not modifications)

**Engine query patterns:**
- Bulk read: load all bonds for a specific document
- Bulk write: store all bonds for a newly ingested document
- Document identity: needs doc_id linkage

### 4. Aggregate PBM Bonds (cross-document, language shard)

**Data:** (token_id_a, token_id_b) → aggregate_count
- Cumulative force data across all ingested documents
- Same structure as per-document, but summed
- This is the inference model — the "language knowledge"

**Characteristics:**
- Incrementally updated when documents are added or removed
- Token-by-token add/remove from inference (Patrick's note: "pbms can be added or removed from an inference token by token")
- Growing — every new document contributes

**Engine query patterns:**
- Bulk read: load full aggregate for simulation force constants
- Incremental update: add/subtract bond counts when documents enter/leave

## Format Notes

- Patrick said this largely reverts to what we had before (PBM-first storage)
- Byte→char and char→word are new tables (we never had sub-word PBM storage)
- Word→sequence per-document replaces the position-based doc_token_positions table
- Key format for char→word bonds: character token IDs preferred (consistent with everything else being token-ID-keyed), but char values work too — your call
- All bond data is directional: (a,b) and (b,a) are separate entries with separate counts
