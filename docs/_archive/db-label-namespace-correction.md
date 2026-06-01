# DB Correction: Label Namespace Migration

**Date:** 2026-03-11
**For:** DB specialist
**Context:** Resolution phase 0 checks out-of-place capitals against the Labels (AD namespace) pool. The pool is sparse because many proper nouns landed in AB namespace without a PoS tag and were never moved to AD.

---

## The Rule

- **AD namespace** = pure Labels — words whose only meaning is a proper noun (names, places, etc.). The namespace itself signals N_PROPER; no other word sense exists.
- **AB namespace** = words with a delta — they have a proper noun sense AND at least one other word sense (e.g. "john": name + slang senses). These stay in AB and carry an explicit `N_PROPER` entry in `token_pos`.
- A `token_pos` / gloss entry exists only when there is a **delta** from the base meaning. For most Labels the name IS the meaning — no tag needed beyond namespace placement.

---

## Work Required

### 1. Move no-PoS AB tokens to AD namespace

Any token currently in the AB namespace that has **no `token_pos` row at all** is a pure Label that was not correctly placed. These should be:

1. Moved to the AD namespace (new `token_id` generated in AD sequence)
2. Given a `token_pos` entry with `pos = 'N_PROPER'` and `cap_property = 'start_cap'`
3. The old AB token removed (or aliased if anything references the old token_id)

**Query to identify candidates:**
```sql
SELECT t.token_id, t.name
FROM tokens t
WHERE t.ns = 'AB'
  AND NOT EXISTS (
    SELECT 1 FROM token_pos tp WHERE tp.token_id = t.token_id
  )
ORDER BY t.name;
```

### 2. AB tokens with N_PROPER PoS — leave in place

Tokens in AB that already have `token_pos.pos = 'N_PROPER'` are intentional delta cases (the name sense coexists with other senses). These stay in AB. No action needed.

### 3. Verify AD namespace tokens have N_PROPER PoS

All existing AD namespace tokens should have `token_pos.pos = 'N_PROPER'`. Verify and fill any gaps:

```sql
SELECT t.token_id, t.name
FROM tokens t
WHERE t.ns = 'AD'
  AND NOT EXISTS (
    SELECT 1 FROM token_pos tp
    WHERE tp.token_id = t.token_id AND tp.pos = 'N_PROPER'
  );
```

---

## Effect on Resolution

Once complete, the envelope query for phase 0 will be updated to pull AD namespace directly (`WHERE ns = 'AD'`) in addition to the existing `WHERE tp.pos = 'N_PROPER'` join. This gives phase 0 a complete candidate pool without relying on PoS tagging being exhaustive.

---

## Notes

- Do **not** move AB tokens that have any `token_pos` row — even if one of their senses is N_PROPER, the delta exists and they belong in AB.
- The AD namespace sequence and token_id format should follow the same pattern as Pass 6 (migration 032 / Pass 6 script).
- After the migration, the engine envelope needs to be reactivated to reload LMDB with the updated AD pool.
