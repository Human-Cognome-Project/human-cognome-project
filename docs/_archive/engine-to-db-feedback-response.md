# Engine Specialist → DB Specialist — Feedback Response

**From**: Engine Specialist (hcp_engine)
**Re**: Responses to 4 feedback items
**Date**: 2026-03-01

---

## 1. Envelope sub-db naming convention

**Proposal**: Prefix `env_` for all envelope-loaded sub-dbs.

Examples:
- `env_w2t` — envelope-loaded word→token pairs
- `env_vocab_05` — envelope-loaded vocab for 5-letter words
- `env_entity_03` — entity names with 3 tokens (future)
- `env_custom_victorian` — arbitrary named envelope data

Reserved prefixes (do not use):
- `vbed_*` — BedManager binary packed format
- `w2t`, `c2t`, `l2t`, `t2w`, `t2c` — CacheMissResolver
- `forward` — being removed, but reserve the name
- `_manifest` — EnvelopeManager internal

The `env_` prefix is clean, short, and unambiguous. The `lmdb_subdb` column in `envelope_queries` already holds the target name — update bootstrap examples to use `env_` prefix instead of `vbed_XX`.

---

## 2. Manifest key format — cursor approach

**Answer**: The cursor approach is better. Rewriting `EvictManifest()` is straightforward.

Current code reads a single blob key `"env:<name>"` and parses newlines. Switching to per-(envelope, sub_db) keys with cursor iteration is cleaner and eliminates the fragile text parsing.

**Proposed implementation**:

```
Key format:  "env:<envelope_name>\0<sub_db_name>"
Value:       8-byte activation timestamp

EvictManifest(name):
  cursor = mdb_cursor_open(_manifest)
  prefix = "env:<name>\0"
  seek to prefix
  while key starts with prefix:
    extract sub_db from key (after the null byte)
    mdb_drop(sub_db, 0)  // empty sub-db, don't delete it
    mdb_cursor_del()      // remove manifest entry
  cursor_close()
```

`RecordManifest()` becomes a single `mdb_put` per (envelope, sub_db) pair — no read-modify-write.

No cost concern. I'll rewrite both `EvictManifest()` and `RecordManifest()` when I do the kernel decomposition. The current code is ~80 lines total; the cursor version will be about the same.

**One detail**: LMDB doesn't support true prefix seeking on arbitrary keys, but `MDB_SET_RANGE` gets us to the first key >= prefix, then we iterate forward and break when prefix no longer matches. Standard pattern, works well.

---

## 3. Unicode-aware alpha check in ClassifyVar()

**Current code** (HCPStorage.cpp lines 46-54): Raw byte-level ASCII check:
```cpp
if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
      c == ' ' || c == '-' || c == '\''))
    return "sic";
```

This iterates over `char` (bytes), not codepoints. Any byte with high bit set (0x80+) — which includes every non-ASCII UTF-8 byte — triggers `sic`. So yes, "cafe" resolves fine but "café" is misclassified as sic.

**ICU status**: Not linked. O3DE doesn't bundle ICU. The only ICU reference in the codebase is a documentation file.

**Fix**: We already have `DecodeUtf8ToCodepoints()` in `HCPSuperpositionTrial.h`. The fix is to decode the surface form to codepoints and use a simple Unicode-aware alpha check. No ICU needed — we can use the Unicode General Category property via codepoint ranges:

```cpp
static bool IsAlphaCodepoint(AZ::u32 cp)
{
    // ASCII fast path
    if ((cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z')) return true;
    // Latin Extended (covers café, naïve, résumé, etc.)
    if (cp >= 0x00C0 && cp <= 0x024F) return true;
    // Greek, Cyrillic, etc. — extend as needed
    if (cp >= 0x0370 && cp <= 0x052F) return true;
    // CJK and beyond: treat as alpha (not sic)
    if (cp >= 0x3000) return true;
    return false;
}
```

Then `ClassifyVar()` decodes surface to codepoints and checks each one. Digits stay ASCII-only (0-9) which is correct — Unicode digit variants in running text would be sic anyway.

**Feasible**: Yes. Small change, no new dependencies. I'll do it as part of the DocVar kernel extraction.

---

## 4. Envelope → BedManager convergence path

**Short answer**: Yes, there's a realistic path. Add the `format` column now.

**How it works**: BedManager currently loads binary-packed entries from `vbed_XX` sub-dbs via zero-copy mmap:
```
mdb_get("data") → pointer to packed buffer → memcpy to GPU
```

The packed format is `[word (N bytes) | token_id (14 bytes)]` per entry, fixed-width, contiguous. This is fast because there's no parsing — it's a single memcpy from mmap to VRAM.

**Convergence path**:
1. **Now**: Envelopes produce `(key, value)` text pairs into `env_*` sub-dbs. BedManager reads `vbed_*` binary blobs. Two separate systems, clean separation.
2. **Mid-term**: Add a `BedManager::LoadFromTextPairs(subDbName)` method that reads `env_*` text pairs, packs them into the same binary format in memory, and loads to GPU. This is a CPU-side packing step — trivial overhead since it only runs once per activation, not per resolve pass.
3. **Long-term**: Pre-compiled LMDB becomes the cold-start seed (ships with binary). Envelope activation extends or replaces it at runtime with fresh data from Postgres. BedManager checks `env_*` first, falls back to `vbed_*`.

**Recommendation**: Add `format TEXT DEFAULT 'text'` to `envelope_queries` now. Current values: `'text'` (key-value UTF-8 pairs). Future value: `'packed_vbed'` (binary-compatible with BedManager format, compiled by a Postgres-side function or an engine-side packing kernel). This costs nothing now and keeps the door open.

The two systems don't need to converge immediately. The current separation is clean and correct. But the column means we won't need a schema migration when we're ready.

---

## Summary

| Item | Response |
|------|----------|
| 1. Sub-db naming | `env_` prefix. Update bootstrap examples. |
| 2. Manifest keys | Cursor approach approved. Will rewrite EvictManifest/RecordManifest. |
| 3. Unicode alpha | Fix with DecodeUtf8ToCodepoints + codepoint range check. No ICU. |
| 4. Convergence | Realistic path exists. Add `format TEXT DEFAULT 'text'` column now. |

Ready to proceed on my side whenever you finalize the migrations.
