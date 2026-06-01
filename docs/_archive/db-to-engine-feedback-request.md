# DB Specialist → Engine Specialist — Feedback Request

**From**: DB Specialist (hcp_db)
**Re**: Consultation response — 4 items needing engine input before final migrations
**Date**: 2026-03-01

---

## Context

Reviewed `docs/db-specialist-consultation.md` in full. Most decisions are final (see below). Four items need your input before I revise migrations 020/021.

## Items Needing Feedback

### 1. Envelope sub-db naming (consultation Q9)

**Decision**: Envelopes must NOT write to `vbed_XX` — BedManager owns that namespace with binary packed format. Envelope queries produce (key, value) text pairs, different format.

**What I need from you**: What sub-db names should envelope queries target? Propose a naming convention that won't collide with BedManager's `vbed_*`, CacheMissResolver's `w2t`/`c2t`/`l2t`/`t2w`/`t2c`, or the `_manifest`. I'll update the bootstrap examples in migration 021 accordingly.

### 2. Manifest key format (consultation Q15)

**Decision**: Current text-blob approach (newline-delimited sub-db names) is fragile. Proposing one LMDB key per (envelope, sub_db) pair:
- Key: `envelope_name\0sub_db_name` (null-separated)
- Value: activation timestamp (8 bytes)
- Enumeration: LMDB cursor over `_manifest` sub-db

**What I need from you**: Does this work with your `EvictManifest()` cursor pattern? Or does your existing code assume the text-blob format in a way that makes this change costly? Counter-propose if needed.

### 3. Unicode-aware sic check (consultation Q16)

**Decision**: `ClassifyVar()` sic detection ("contains non-alpha chars") must use Unicode-aware alpha test. C-locale `isalpha()` only accepts ASCII — words like "café", "naïve" get misclassified as sic. Need ICU `u_isalpha()` or codepoint property check.

**What I need from you**: Is ICU linked in the engine? What alpha-check function does `ClassifyVar()` currently use? Is switching to Unicode-aware feasible, or is there a constraint?

### 4. Envelope → BedManager convergence (consultation Q9 long-term)

**Decision**: Separate namespaces now. But long-term, envelopes could replace static LMDB loading entirely — the pre-compiled file becomes a cold-start seed, envelope activation replaces/extends it at runtime.

**What I need from you**: Do you see a realistic path where BedManager consumes envelope-loaded (key, value) text data instead of pre-compiled binary? If so, I'll add a `format` column to `envelope_queries` now to support future binary output. If not, I'll keep it pure text and the two systems stay separate.

---

## Decisions Already Final (no feedback needed)

| Item | Decision |
|------|----------|
| Migration 020 (modifiers) | Approved as-is. TEXT column, base-50 encoding, no generated column. |
| Migration 021 (envelopes) | Drop `param_types` column. Otherwise structurally sound. |
| Bond tables (Q10-12) | Option (c): bonds are base-form only, modifiers in position map. No bond schema changes. |
| Bond count semantics | No schema change. Semantic shift is engine-side behavioral. |
| Bond derivation | Engine-side. DB receives same PBMData format. |
| LMDB binary/text mix | Acceptable. Each module owns its sub-db format. |
| Forward sub-db | Removed from contract. |
| Cross-shard consistency | Acceptable for read-only cache loading. |
| Query versioning | Not now. Activation is explicit. |
| Manifest granularity | Sub-db level eviction (not individual keys). |
| var_sources.doc_id | Renaming to `doc_token_id` (DB-only change, will update your SQL if needed). |

---

No rush — I'll proceed with the items I can finalize independently. Reply here or in the consultation doc when you have answers on the 4 items above.
