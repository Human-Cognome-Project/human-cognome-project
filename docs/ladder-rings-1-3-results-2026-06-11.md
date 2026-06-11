# Ladder Conversion — Rings 1-3 Results

2026-06-11 — continuation of [sense-conversion-pilot-results-2026-06-11.md](sense-conversion-pilot-results-2026-06-11.md).
Autonomous block under Patrick's go-ahead: preflight fixes applied, then the ladder run outward
three rings from core. Everything is additive (`cx_` tables in hcp_english) and deterministically
rebuildable from scratch — rollback = drop the tables and re-run.

## What a "ring" is

Ring N = every word that the formulas of rings 0..N-1 *use* as content but that isn't a concept
yet. This is the data-driven version of the LTWF ladder: instead of taking the published list,
the glosses themselves tell us which words they need, weighted by demand.

## Preflight fixes applied this block

- **Form-of and dated senses excluded** ("past participle of X", obsolete/archaic/dated tags —
  525k + 75k senses preflight-skipped corpus-wide). Form descriptions are not concepts; they
  belong to the forms/inflection machinery.
- **Lemma fix table** for tokenizer short-word mis-links (withe→with, gyve→give).
- **Gloss-ese scaffold extension** (pertaining, denoting, specified, sometimes).
- Mint-only policy at scale: no auto-merge (bag keys under-discriminate); duplicate mints are
  recoverable, wrong merges are not.

## Results

| ring | words converted | senses | concepts minted |
|---|---|---|---|
| seam (core 311) | 306 linked | 1,137 | 740 (seeded) |
| ring 1 | 608 | 2,077 | 1,541 |
| ring 2 | 1,036 | 3,219 | 2,442 |
| ring 3 | 1,323 | 3,713 | 2,686 |
| **total** | **~3,270** | **10,146** | **7,409** |

2,899 distinct words now carry at least one concept skin.

## Completeness (the audit metric: a sense is COMPLETE when every content word in its gloss is
itself a concept)

- Overall: 47.4% complete (was 27.4% before rings 2-3).
- Innermost (seam): 50% → 57% → **62%** complete as each ring lands — inner rings close as outer
  rings convert, exactly the predicted ladder behavior.
- Per-ring at time of audit: ring1 55%, ring2 51%, ring3 36% (each newest ring is rawest).

## Notable

- **The demand-driven ladder reproduced Patrick's "first 2000" estimate**: rings 0-2 = 311 + 608
  + 1,036 ≈ 1,955 words — the size of the LTWF/Longman defining vocabulary, arrived at purely by
  letting glosses demand their definienda. Cross-checking the actual word overlap against the
  published lists is an open TODO.
- Top frontier demand at ring 1 was the **metaclass vocabulary** (physical, abstract, object,
  substance, human, form, force) — the class-map top slice (claim 507) surfacing as demand.
- Trend says full closure of the reachable lowercase-common-word component is tractable: frontier
  growth per ring is slowing relative to senses converted; plausibly ~10-20k words total.

## Open queue

1. Ring 4+ to closure, or stop and do CEF-structured keys first (better keys before more volume).
2. Post-hoc dedup pass over the 7,409 mints (identical-key merging already happens; near-key is
   the CEF job).
3. LTWF/Longman overlap check.
4. Promotion of the pilot registry to the real concept shard — Patrick's sign-off (claim 533
   migration).
